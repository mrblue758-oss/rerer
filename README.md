# Auto Deco (Geode) — v0.1 · estilo Nine Circles

Boton **Auto Deco** dentro del editor (menu de pausa, esquina inferior izquierda) que decora el nivel.
Tambien hay **Quitar deco** para sacar todo lo que puso el mod.

## Que hace (y que no)
- Solo agrega **deco** y **triggers de color (899)**. Despues de crear cada tanda, cualquier objeto
  que GD no clasifique como decoracion se borra al instante (red de seguridad).
- Lee todos los objetos del nivel: celdas ocupadas, canales de color, grupos y capas del editor.
  - No pone deco sobre celdas ocupadas (ni sobre tu deco, si `skip-existing-deco` esta activo).
  - Usa **3 canales de color libres** (no pisa los tuyos) y **4 capas del editor libres**.
  - Marca todo con un grupo (`marker-group`, 9999 por defecto): volver a decorar reemplaza la corrida anterior.
- Sin limite propio de objetos (`max-objects = 0`). Crea en tandas de 1500; en niveles enormes puede congelar unos segundos.
  GD 2.2 todavia avisa en 40k/80k objetos: se desactiva desde los ajustes del editor.

## Estilo Nine Circles v0.1 (aprendido de tu par sin deco / con deco)
- Aureola de 3 piezas (960 + 713 + 1091) sobre bloques 1338 y en celdas libres vecinas (radio 3, cobertura 42/40/32 % como tu deco).
- Hazards 326/328, 327/329 y 1339: pila de piezas en el mismo lugar (recetas medidas).
- Pulso de color: 3 canales que se prenden/apagan en cadena cada 60 unidades (azul, celeste, cian).
- **Hazards con otro ID quedan sin deco** (el mod te avisa cuantos). Para ampliarlo: mas pares sin deco / con deco.
  `python3 tools/analyze_pair.py sin_deco.gmd con_deco.gmd` muestra la receta de cada ID.

## Como obtener el .geode
No se puede compilar sin el SDK de Geode, asi que:
1. Cambia `developer` e `id` en `mod.json` (formato `tunombre.auto-deco`).
2. Subi esta carpeta a un repo de GitHub. **Actions** corre `build.yml` y genera el `.geode`
   (descargalo de *Artifacts → Build Output*).
   O compila local: `geode build` (con el SDK instalado) y queda en `build/`.
3. Copia el `.geode` a la carpeta `geode/mods` de GD (o arrastralo al juego).

## Si no compila
El codigo de Geode es solo `src/main.cpp`; los puntos que dependen de la version de GD estan
marcados `[1]`-`[4]` arriba del archivo. Mandame el log de error y lo ajusto.

## Prueba hecha
`src/Core.cpp` (el generador) no depende de Geode y se probo aparte contra `nivel_de_prueba_sin_deco.gmd`:
sin superposiciones con gameplay, sin piezas duplicadas, resultado determinista por semilla,
cobertura de aureola 43/40/37 % (referencia 42/40/32 %). **No probado dentro de GD** (no hay forma de correrlo aca).
