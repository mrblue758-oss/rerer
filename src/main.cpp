// Auto Deco v0.1 -- boton dentro del editor (menu de pausa) que decora el nivel
// con el estilo "Nine Circles". SOLO agrega deco + triggers de color; nunca objetos de gameplay.
//
// PUNTOS SENSIBLES A LA VERSION (si el compilador se queja, es aca y solo aca):
//   [1] GameObject::getSaveString(GJBaseGameLayer*)
//   [2] LevelEditorLayer::createObjectsFromString(gd::string, bool, bool)
//   [3] LevelEditorLayer::removeObject(GameObject*, bool)
//   [4] GameObject::m_objectType / GameObjectType::{Solid,Hazard,Decoration}
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>

#include <algorithm>
#include <vector>

#include "Core.hpp"

using namespace geode::prelude;

namespace {

autodeco::Kind kindOf(GameObject* o) {
    switch (o->m_objectType) {                                   // [4]
        case GameObjectType::Solid:      return autodeco::Kind::Solid;
        case GameObjectType::Hazard:     return autodeco::Kind::Hazard;
        case GameObjectType::Decoration: return autodeco::Kind::Deco;
        default:                         return autodeco::Kind::Other;
    }
}

std::string saveStringOf(LevelEditorLayer* lel, GameObject* o) {
    return std::string(o->getSaveString(lel));                   // [1]
}

std::vector<autodeco::Obj> readLevel(LevelEditorLayer* lel) {
    std::vector<autodeco::Obj> v;
    for (auto o : CCArrayExt<GameObject*>(lel->m_objects)) {
        v.push_back(autodeco::fromKV(autodeco::parseSaveString(saveStringOf(lel, o)), kindOf(o)));
    }
    return v;
}

// Borra lo que agrego una corrida anterior (todo lo marcado con el grupo marcador).
int removeMarked(LevelEditorLayer* lel, int marker) {
    std::vector<GameObject*> kill;
    for (auto o : CCArrayExt<GameObject*>(lel->m_objects)) {
        if (autodeco::hasGroup(autodeco::parseSaveString(saveStringOf(lel, o)), marker)) kill.push_back(o);
    }
    for (auto o : kill) lel->removeObject(o, true);              // [3]
    return (int)kill.size();
}

// Crea una tanda. Red de seguridad: si algo creado NO es deco (ni trigger de color 899),
// se borra al instante y se cuenta.
int spawnChunk(LevelEditorLayer* lel, std::string const& chunk, int& rejected) {
    CCArray* made = lel->createObjectsFromString(chunk, true, true);  // [2]
    if (!made) return 0;
    std::vector<GameObject*> bad;
    int ok = 0;
    for (auto o : CCArrayExt<GameObject*>(made)) {
        if (o->m_objectType != GameObjectType::Decoration && o->m_objectID != 899) bad.push_back(o);
        else ++ok;
    }
    for (auto o : bad) lel->removeObject(o, true);
    rejected += (int)bad.size();
    return ok;
}

autodeco::Options readOptions() {
    auto* m = Mod::get();
    autodeco::Options o;
    o.seed             = (unsigned)m->getSettingValue<int64_t>("seed");
    o.colorChase       = m->getSettingValue<bool>("color-chase");
    o.chaseStep        = (float)m->getSettingValue<int64_t>("chase-step");
    o.maxObjects       = (int)m->getSettingValue<int64_t>("max-objects");   // 0 = sin limite
    o.skipExistingDeco = m->getSettingValue<bool>("skip-existing-deco");
    o.markerGroup      = (int)m->getSettingValue<int64_t>("marker-group");
    o.haloDensity      = (float)m->getSettingValue<double>("halo-density");
    return o;
}

void alert(std::string const& title, std::string const& body) {
    FLAlertLayer::create(title.c_str(), body, "OK")->show();
}

} // namespace

class $modify(AutoDecoPause, EditorPauseLayer) {
    bool init(LevelEditorLayer* editor) {
        log::info("[Auto Deco] EditorPauseLayer::init hook entered");
        if (!EditorPauseLayer::init(editor)) {
            log::error("[Auto Deco] Original EditorPauseLayer::init returned false");
            return false;
        }

        auto menu = CCMenu::create();
        menu->setID("auto-deco-menu"_spr);
        menu->setPosition({82.f, 82.f});          // esquina inferior izquierda; separado del borde
        menu->setContentSize({110.f, 70.f});
        menu->setVisible(true);
        menu->setOpacity(255);
        menu->setZOrder(999);
        menu->setLayout(ColumnLayout::create()->setAxisReverse(true)->setGap(6.f));

        auto mk = [&](char const* text, SEL_MenuHandler cb) {
            auto spr = ButtonSprite::create(text, 100, true, "bigFont.fnt", "GJ_button_04.png", 28.f, .55f);
            auto btn = CCMenuItemSpriteExtra::create(spr, this, cb);
            menu->addChild(btn);
        };
        mk("Auto Deco", menu_selector(AutoDecoPause::onAutoDeco));
        mk("Quitar deco", menu_selector(AutoDecoPause::onRemoveDeco));
        menu->updateLayout();
        this->addChild(menu, 999);
        log::info("[Auto Deco] Added pause-menu buttons: Auto Deco / Quitar deco");
        return true;
    }

    void onAutoDeco(CCObject*) {
        log::info("[Auto Deco] Auto Deco button clicked");
        createQuickPopup(
            "Auto Deco",
            "Estilo: <cy>Nine Circles</c> (v0.1)\n\n"
            "Solo agrega <cg>deco y triggers de color</c>. No toca ni agrega objetos de gameplay "
            "y evita las celdas ocupadas. Si ya habias decorado antes, lo reemplaza.\n\n"
            "<cy>Guarda tu nivel antes</c> por las dudas.",
            "Cancelar", "Decorar",
            [this](FLAlertLayer*, bool ok) { if (ok) this->runDeco(); });
    }

    void onRemoveDeco(CCObject*) {
        auto opt = readOptions();
        int n = removeMarked(m_editorLayer, opt.markerGroup);
        alert("Auto Deco", fmt::format("Se quitaron {} objetos agregados por el mod.", n));
    }

    void runDeco() {
        auto lel = m_editorLayer;
        auto opt = readOptions();

        int removed = removeMarked(lel, opt.markerGroup);        // idempotente
        auto res = autodeco::generate(readLevel(lel), opt);
        if (!res.error.empty()) { alert("Auto Deco", res.error); return; }

        int created = 0, rejected = 0;
        for (auto const& chunk : res.chunks) created += spawnChunk(lel, chunk, rejected);

        std::string msg = fmt::format(
            "Listo: <cg>{}</c> objetos de deco y triggers (<cy>{}</c> triggers de color).\n"
            "Canales de color usados: {}, {}, {}  |  Capas del editor: {}-{}\n"
            "Activa <cy>Preview Mode</c> en el menu de pausa para ver los colores.",
            created - res.triggers, res.triggers, res.channels[0], res.channels[1], res.channels[2],
            res.layerBase, res.layerBase + 3);
        if (removed) msg += fmt::format("\nSe reemplazaron {} objetos de una corrida anterior.", removed);
        if (res.skippedHazards)
            msg += fmt::format("\n<cy>{}</c> hazards sin receta aprendida quedaron sin deco "
                               "(IDs distintos: {}). Mandame mas pares sin deco/con deco.",
                               res.skippedHazards, res.skippedIds.size());
        if (res.truncated) msg += "\nSe corto por el tope 'max-objects' de los ajustes.";
        if (rejected) msg += fmt::format("\n<cr>{} objetos no eran deco y se descartaron.</c>", rejected);
        alert("Auto Deco", msg);
    }
};
