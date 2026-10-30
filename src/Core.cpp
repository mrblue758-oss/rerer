#include "Core.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <map>

namespace autodeco {
namespace {

// ---------------------------------------------------------------------------
// ESTILO "NINE CIRCLES" v0.1 -- aprendido de nivel_de_prueba_sin_deco.gmd vs
// NIVEL_DE_PRUEBA_DECORADO.gmd (ver tools/analyze_pair.py). Cada pieza:
// {id de objeto, z-layer (key 24), editor layer (key 20), usa canal detalle (key 22)}
// ---------------------------------------------------------------------------
struct Piece { int id; int z; int editorLayer; bool detail; };

// Aureola: se apila sobre bloques solidos y en las celdas libres vecinas.
const std::vector<Piece> HALO  = {{960, -1, 1, true}, {713, 1, 2, false}, {1091, 3, 3, true}};
// Hazards (mismo lugar, mismo flip/rotacion +0/180):
const std::vector<Piece> HAZ_A = {{1091, 0, 2, true}, {693, -3, 1, false}};              // 326, 328
const std::vector<Piece> HAZ_B = {{1092, 0, 4, true}, {694, -3, 2, false}};              // 327, 329
const std::vector<Piece> HAZ_C = {{1092, 5, 0, true}, {961, -1, 0, true}, {714, 0, 0, false}}; // 1339

const std::vector<Piece>* hazardRecipe(int id) {
    switch (id) {
        case 326: case 328: return &HAZ_A;
        case 327: case 329: return &HAZ_B;
        case 1339:          return &HAZ_C;
        default:            return nullptr;
    }
}
// Bloque que ademas recibe la pila encima de si mismo (como en tu nivel de prueba).
bool stacksOnSelf(int id) { return id == 1338; }

// Cobertura de aureola medida en tu nivel decorado, por distancia (Chebyshev) al bloque
// solido mas cercano: anillo 1 = 42%, anillo 2 = 40%, anillo 3 = 32%. Encima del bloque = 100%.
const float HALO_P[4] = {1.0f, 0.42f, 0.40f, 0.32f};
const int HALO_RADIUS = 3;

struct RGB { int r, g, b; };
const RGB PAL[3] = {{17, 0, 255}, {0, 147, 255}, {0, 238, 255}};  // azul, celeste, cian

using Cell = std::pair<int, int>;
int cellOf(float v) { return (int)std::floor(v / 30.f); }

uint64_t mix(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}
uint64_t siteHash(unsigned seed, int cx, int cy, int salt) {
    return mix(((uint64_t)seed << 40) ^ ((uint64_t)(uint32_t)cx << 20) ^ (uint64_t)(uint32_t)cy ^ ((uint64_t)salt << 56));
}
// Pesos aprendidos: ~47% / 32% / 21% entre los 3 canales.
int pickChannelIdx(uint64_t h) {
    int r = (int)(h % 100);
    return r < 47 ? 0 : (r < 79 ? 1 : 2);
}

std::string num(float v) {
    char b[48];
    if (std::fabs(v - std::round(v)) < 1e-4f) std::snprintf(b, sizeof b, "%d", (int)std::lround(v));
    else std::snprintf(b, sizeof b, "%g", v);
    return b;
}

void emitPiece(std::string& s, Piece const& p, int editorLayer, float x, float y, bool fx, bool fy,
               int rot, int ch, int group) {
    s += "1," + std::to_string(p.id) + ",2," + num(x) + ",3," + num(y);
    if (fx) s += ",4,1";
    if (fy) s += ",5,1";
    if (rot % 360 != 0) s += ",6," + std::to_string(((rot % 360) + 360) % 360);
    if (editorLayer) s += ",20," + std::to_string(editorLayer);
    if (p.z) s += ",24," + std::to_string(p.z);
    s += ",21," + std::to_string(ch);
    if (p.detail) s += ",22," + std::to_string(ch);
    s += ",57," + std::to_string(group) + ";";
}

void emitColorTrigger(std::string& s, float x, float y, int ch, RGB c, float fade, int group) {
    // mismos campos que los 899 de tu nivel decorado: 7/8/9 RGB, 10 fade, 35 opacidad, 23 canal
    s += "1,899,2," + num(x) + ",3," + num(y) + ",36,1,7," + std::to_string(c.r) + ",8," +
         std::to_string(c.g) + ",9," + std::to_string(c.b) + ",10," + num(fade) + ",35,1,23," +
         std::to_string(ch) + ",57," + std::to_string(group) + ";";
}

} // namespace

// ---------------------------------------------------------------------------
std::map<int, std::string> parseSaveString(std::string const& str) {
    std::map<int, std::string> kv;
    std::string s = str;
    while (!s.empty() && (s.back() == ';' || s.back() == '\n' || s.back() == '\r')) s.pop_back();
    std::vector<std::string> tok;
    size_t i = 0;
    while (i <= s.size()) {
        size_t j = s.find(',', i);
        if (j == std::string::npos) j = s.size();
        tok.push_back(s.substr(i, j - i));
        i = j + 1;
    }
    for (size_t k = 0; k + 1 < tok.size(); k += 2) {
        char* end = nullptr;
        long key = std::strtol(tok[k].c_str(), &end, 10);
        if (end == tok[k].c_str()) continue;
        kv[(int)key] = tok[k + 1];
    }
    return kv;
}

static void addInts(std::vector<int>& out, std::string const& v, bool dotted) {
    if (dotted) {
        size_t i = 0;
        while (i <= v.size()) {
            size_t j = v.find('.', i);
            if (j == std::string::npos) j = v.size();
            int n = std::atoi(v.substr(i, j - i).c_str());
            if (n > 0) out.push_back(n);
            i = j + 1;
        }
    } else {
        int n = std::atoi(v.c_str());
        if (n > 0) out.push_back(n);
    }
}

Obj fromKV(std::map<int, std::string> const& kv, Kind kind) {
    Obj o;
    auto get = [&](int k) -> std::string { auto it = kv.find(k); return it == kv.end() ? "" : it->second; };
    o.id = std::atoi(get(1).c_str());
    o.x = (float)std::atof(get(2).c_str());
    o.y = (float)std::atof(get(3).c_str());
    o.flipX = get(4) == "1";
    o.flipY = get(5) == "1";
    o.rot = (float)std::atof(get(6).c_str());
    o.kind = kind;
    addInts(o.groups, get(57), true);
    for (int k : {51, 71, 401}) addInts(o.groups, get(k), false);
    for (int k : {21, 22, 23, 50}) addInts(o.channels, get(k), false);
    for (int k : {20, 61}) addInts(o.editorLayers, get(k), false);
    return o;
}

bool hasGroup(std::map<int, std::string> const& kv, int group) {
    auto it = kv.find(57);
    if (it == kv.end()) return false;
    std::vector<int> g;
    addInts(g, it->second, true);
    return std::find(g.begin(), g.end(), group) != g.end();
}

// ---------------------------------------------------------------------------
Output generate(std::vector<Obj> const& objs, Options const& opt) {
    Output out;
    if (objs.empty()) { out.error = "El nivel esta vacio."; return out; }

    // 1) Lectura: ocupacion de celdas, canales/grupos en uso, limites del nivel.
    std::set<Cell> occupied;
    std::set<int> usedCh, usedGr, usedLayers;
    float minX = 1e9f, maxX = -1e9f, maxY = -1e9f;
    for (auto const& o : objs) {
        if (o.kind != Kind::Deco || opt.skipExistingDeco)
            occupied.insert({cellOf(o.x), cellOf(o.y)});
        for (int c : o.channels) usedCh.insert(c);
        for (int g : o.groups) usedGr.insert(g);
        for (int l : o.editorLayers) usedLayers.insert(l);
        minX = std::min(minX, o.x); maxX = std::max(maxX, o.x); maxY = std::max(maxY, o.y);
    }
    if (usedGr.count(opt.markerGroup)) {
        out.error = "El grupo marcador " + std::to_string(opt.markerGroup) +
                    " ya esta en uso en este nivel. Cambialo en los ajustes del mod.";
        return out;
    }

    // 2) Tres canales de color LIBRES (nunca pisamos los del usuario).
    int ch[3], found = 0;
    for (int c = 10; c < 1000 && found < 3; ++c)
        if (!usedCh.count(c)) ch[found++] = c;
    if (found < 3) { out.error = "No hay 3 canales de color libres."; return out; }
    out.channels = {ch[0], ch[1], ch[2]};

    // 2b) Capas del editor: 4 capas consecutivas que NO use ningun objeto del nivel.
    int layerBase = 1;
    for (;; ++layerBase) {
        bool free = true;
        for (int l = layerBase; l < layerBase + 4; ++l) if (usedLayers.count(l)) { free = false; break; }
        if (free || layerBase > 9990) break;
    }
    out.layerBase = layerBase;

    // 3) Deco por objeto.
    std::string cur;
    int curCount = 0;
    auto flush = [&]() { if (!cur.empty()) { out.chunks.push_back(cur); cur.clear(); curCount = 0; } };
    auto budgetLeft = [&](int n) {
        return opt.maxObjects <= 0 || out.decoObjects + out.triggers + n <= opt.maxObjects;
    };
    auto addStack = [&](std::vector<Piece> const& pieces, float x, float y, bool fx, bool fy,
                        int rotBase, bool randRot90, bool randRot180, int salt) -> bool {
        if (!budgetLeft((int)pieces.size())) { out.truncated = true; return false; }
        int cx = cellOf(x), cy = cellOf(y);
        uint64_t h = siteHash(opt.seed, cx, cy, salt);
        int c = ch[pickChannelIdx(h)];        // una sola tinta por pila, como en tu deco
        int rot = rotBase;
        if (randRot90) rot += 90 * (int)((h >> 8) % 4);
        else if (randRot180) rot += 180 * (int)((h >> 8) % 2);
        for (auto const& p : pieces) {
            emitPiece(cur, p, p.editorLayer ? layerBase + p.editorLayer - 1 : 0, x, y, fx, fy, rot, c,
                      opt.markerGroup);
            ++out.decoObjects; ++curCount;
        }
        if (curCount >= opt.chunkSize) flush();
        return true;
    };

    std::set<std::pair<Cell, int>> stackedAt;  // (celda, receta) ya apilados: evita duplicados
    std::set<Cell> placed;                     // celdas donde ya hay aureola
    std::map<Cell, int> haloDist;              // distancia al solido mas cercano (celdas libres)
    bool stop = false;

    // 3a) Recetas aprendidas: hazards y bloque propio (mismo lugar que el objeto).
    for (auto const& o : objs) {
        if (stop) break;
        if (o.kind == Kind::Deco || o.kind == Kind::Other) continue;
        int cx = cellOf(o.x), cy = cellOf(o.y);
        if (auto const* rec = hazardRecipe(o.id)) {
            if (!stackedAt.insert({{cx, cy}, o.id == 1339 ? 3 : (o.id == 326 || o.id == 328 ? 1 : 2)}).second)
                continue;  // 326 y 328 (etc.) en la misma celda: una sola pila
            if (!addStack(*rec, o.x, o.y, o.flipX, o.flipY, (int)o.rot, false, true, 1)) stop = true;
            continue;
        }
        if (o.kind == Kind::Hazard) {  // hazard sin receta: no inventamos, lo reportamos
            ++out.skippedHazards; ++out.skippedIds[o.id];
            continue;
        }
        if (stacksOnSelf(o.id) && !placed.count({cx, cy})) {
            placed.insert({cx, cy});
            if (!addStack(HALO, o.x, o.y, false, false, 0, true, false, 2)) stop = true;
        }
        // 3b) mapa de distancias para la aureola (solo celdas libres)
        for (int dx = -HALO_RADIUS; dx <= HALO_RADIUS; ++dx)
            for (int dy = -HALO_RADIUS; dy <= HALO_RADIUS; ++dy) {
                int d = std::max(std::abs(dx), std::abs(dy));
                if (d == 0) continue;
                Cell c{cx + dx, cy + dy};
                if (c.second < 0 || occupied.count(c)) continue;  // piso / no pisar nada
                auto it = haloDist.find(c);
                if (it == haloDist.end() || d < it->second) haloDist[c] = d;
            }
    }
    // 3c) Aureola: cada celda libre cercana se decide por hash (determinista).
    for (auto const& [c, d] : haloDist) {
        if (stop) break;
        if (placed.count(c)) continue;
        uint64_t h = siteHash(opt.seed, c.first, c.second, 7);
        if ((double)(h % 10000) / 10000.0 >= HALO_P[d] * opt.haloDensity) continue;
        placed.insert(c);
        float px = c.first * 30.f + 15.f, py = c.second * 30.f + 15.f;
        if (!addStack(HALO, px, py, false, false, 0, true, false, 3)) stop = true;
    }

    // 4) Triggers de color (sin tope propio de objetos).
    float rowY = std::ceil((maxY + 120.f) / 15.f) * 15.f;   // fila libre por encima de todo
    const float startX = -29.f;                             // igual que tu nivel decorado
    if (!opt.colorChase) {
        if (budgetLeft(3)) for (int i = 0; i < 3; ++i) {  // colores fijos
            emitColorTrigger(cur, startX, rowY + 30.f * i, ch[i], PAL[i], 0.f, opt.markerGroup);
            ++out.triggers; ++curCount;
        }
    } else if (budgetLeft(3)) {
        for (int i = 0; i < 3; ++i) {  // arranque: los 3 canales en negro
            emitColorTrigger(cur, startX, rowY + 30.f * i, ch[i], RGB{0, 0, 0}, 0.f, opt.markerGroup);
            ++out.triggers; ++curCount;
        }
        // persecucion: en cada paso se prende un canal y se apaga el anterior (orden 1,2,0,...)
        const int seq[3] = {1, 2, 0};
        float step = std::max(15.f, opt.chaseStep);
        int i = 0;
        for (float x = std::max(225.f, std::floor(minX / 15.f) * 15.f); x <= maxX; x += step, ++i) {
            if (!budgetLeft(2)) { out.truncated = true; break; }
            int on = seq[i % 3], off = seq[(i + 2) % 3];
            emitColorTrigger(cur, x, rowY, ch[off], RGB{0, 0, 0}, 0.03f, opt.markerGroup);
            emitColorTrigger(cur, x, rowY + 30.f, ch[on], PAL[on], 0.03f, opt.markerGroup);
            out.triggers += 2; curCount += 2;
            if (curCount >= opt.chunkSize) flush();
        }
    }
    flush();
    return out;
}

} // namespace autodeco
