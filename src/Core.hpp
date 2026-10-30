#pragma once
// Nucleo del generador: NO depende de Geode. Se puede compilar y probar solo.
#include <map>
#include <string>
#include <vector>

namespace autodeco {

enum class Kind { Solid, Hazard, Deco, Other };

// Un objeto del nivel, ya leido (posicion, rotacion, canales y grupos usados).
struct Obj {
    int id = 0;
    float x = 0, y = 0;
    bool flipX = false, flipY = false;
    float rot = 0;
    Kind kind = Kind::Other;
    std::vector<int> groups;    // grupos a los que pertenece o que referencia
    std::vector<int> channels;  // canales de color que usa o referencia
    std::vector<int> editorLayers; // capas del editor (keys 20 y 61)
};

struct Options {
    unsigned seed = 1;
    bool colorChase = true;      // triggers de color estilo Nine Circles
    float chaseStep = 60.f;      // unidades entre pulsos (60 = como en tu nivel de prueba)
    int maxObjects = 0;          // 0 = sin limite
    bool skipExistingDeco = true;// no apilar sobre deco que ya existe
    int markerGroup = 9999;      // grupo que marca todo lo que agrega el mod
    int chunkSize = 1500;        // objetos por tanda al crear en el editor
    float haloDensity = 1.0f;    // multiplica la cobertura de la aureola (1 = como tu nivel)
};

struct Output {
    std::vector<std::string> chunks;  // level strings listos para pegar en el editor
    int decoObjects = 0;
    int triggers = 0;
    int skippedHazards = 0;           // hazards sin receta aprendida
    std::map<int, int> skippedIds;
    std::vector<int> channels;        // canales de color asignados
    int layerBase = 0;                // primera capa del editor usada por la deco (0 = ninguna)
    bool truncated = false;
    std::string error;
};

// "1,1338,2,195,3,945,..." -> {1:"1338", 2:"195", ...}
std::map<int, std::string> parseSaveString(std::string const& s);
Obj fromKV(std::map<int, std::string> const& kv, Kind kind);

bool hasGroup(std::map<int, std::string> const& kv, int group);   // solo key 57

Output generate(std::vector<Obj> const& objs, Options const& opt);

} // namespace autodeco
