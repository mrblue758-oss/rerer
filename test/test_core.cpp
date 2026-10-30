// Prueba offline: lee un level string DECODIFICADO (objetos separados por ';'),
// clasifica por una tabla simple (en el mod real lo hace GD con m_objectType)
// y escribe el resultado a un archivo para verificarlo con test/verify.py
#include "../src/Core.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
using namespace autodeco;

int main(int argc, char** argv) {
    if (argc < 3) { std::cerr << "uso: test_core in.txt out.txt [seed] [chase]\n"; return 1; }
    std::ifstream f(argv[1]); std::stringstream ss; ss << f.rdbuf();
    std::string all = ss.str();
    std::vector<Obj> objs;
    size_t i = all.find(';') + 1;                       // saltar header
    while (i < all.size()) {
        size_t j = all.find(';', i); if (j == std::string::npos) j = all.size();
        std::string s = all.substr(i, j - i); i = j + 1;
        if (s.empty()) continue;
        auto kv = parseSaveString(s);
        int id = std::atoi(kv[1].c_str());
        Kind k = Kind::Other;
        if (id == 1338 || id == 661 || id == 699) k = Kind::Solid;
        else if (id >= 326 && id <= 329) k = Kind::Hazard;
        else if (id == 1339) k = Kind::Hazard;
        else if (id == 1728) k = Kind::Solid;             // para probar el camino generico
        objs.push_back(fromKV(kv, k));
    }
    Options opt; if (argc > 3) opt.seed = std::atoi(argv[3]); if (argc > 4) opt.colorChase = std::atoi(argv[4]);
    auto out = generate(objs, opt);
    if (!out.error.empty()) { std::cerr << "ERROR: " << out.error << "\n"; return 2; }
    std::ofstream o(argv[2]);
    for (auto& c : out.chunks) o << c << "\n";
    std::cout << "deco=" << out.decoObjects << " triggers=" << out.triggers << " skippedHaz="
              << out.skippedHazards << " chunks=" << out.chunks.size() << " ch="
              << out.channels[0] << "," << out.channels[1] << "," << out.channels[2] << "\n";
}
