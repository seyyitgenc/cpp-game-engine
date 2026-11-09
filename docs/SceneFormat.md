# The `.gncscene` format, version 1

A scene file holds one scene: its settings, the meshes and textures it uses,
its materials and its entity tree. The editor saves it, and the engine loads
it on demand in place of the current scene. It is UTF-8 JSON
([RFC 8259](https://www.rfc-editor.org/rfc/rfc8259)) with the extension
`.gncscene`. `resources/scenes/Showcase.gncscene`, the scene the engine
starts with, is an example the engine wrote itself.

`Scene/SceneSerializer` reads and writes it, with
[nlohmann/json](https://github.com/nlohmann/json).

## Why JSON

| Engine | Scene file | Text format |
|---|---|---|
| Unity | `.unity` | A YAML subset with Unity's own tags (`--- !u!1 &123`); objects reference each other by `fileID`, assets by `guid` from `.meta` files |
| Godot | `.tscn` | Godot's own INI-like format: `[ext_resource]`, `[sub_resource]`, then `[node name= parent=]` sections |
| Unreal | `.umap` | Binary. "One File Per Actor" splits a level into one file per actor so people can merge, but each file is still binary |
| O3DE | `.prefab` | JSON |
| Bevy | `.scn.ron` | RON, Rust's object notation |
| glTF | `.gltf` | JSON |

All of them keep the editor's file as text, or split it up, because people
diff and merge scenes. None that uses YAML uses all of YAML: Unity writes and
reads a subset of its own. JSON fits this engine best:

- **It is what glTF uses**, and the engine already follows glTF's semantics for
  materials, lights and cameras, so the same names (`baseColorFactor`,
  `yfov`, `KHR_lights_punctual` units) carry over unchanged.
- **One way to write each value.** YAML guesses types from the text (`no` and
  `off` read as false in YAML 1.1, `1.10` as the number 1.1), and indentation is
  part of the syntax. A JSON file is either well formed or rejected.
- **Libraries:** nlohmann/json (one header, the simplest API), RapidJSON or
  glaze (faster). yaml-cpp is slow and big for what it does.
- **Tooling:** every editor, language and diff tool reads it, as do schema
  validators, should the format ever get a JSON Schema.

What JSON costs: no comments and more punctuation than YAML. A scene is
written by the editor, not by hand, so neither matters much.

A binary format can come later for shipped games, cooked from this one, as
Godot does with `.scn` and as the TODO list's asset cooking plans.

## Top level

```json
{
  "format": "gncscene",
  "version": 1,
  "name": "Showcase",
  "dimension": "3D",
  "camera": "7efadd669ab6d12c",
  "environment": { ... },
  "meshes": { ... },
  "textures": { ... },
  "materials": { ... },
  "entities": [ ... ]
}
```

| Key | Value |
|---|---|
| `format` | Always `"gncscene"`. Tells a scene file from any other JSON. |
| `version` | The format version, an integer. This document is version 1. |
| `name` | The scene's name, shown in the window title. Saving under a new file name renames the scene after the file. |
| `dimension` | `"2D"` or `"3D"`; see below. |
| `camera` | The id of the entity whose `Camera` the scene is seen through, or `null`; see Cameras. |
| `environment` | Scene-wide render settings. |
| `meshes` | Every mesh the scene uses, by resource id. |
| `textures` | Every texture its materials use, by resource id. |
| `materials` | The scene's materials, by material id. |
| `entities` | The root entities, in order, each holding its children. |

## 2D and 3D scenes

Both kinds use the same entities, components and file layout; `dimension`
only says how the editor treats the scene. This is Unity's approach: a 2D
scene is a 3D scene seen down one axis, with the same `Transform`. Godot
instead has separate 2D node types (`Node2D` and its `Transform2D`), which
here would mean a second transform, culling, picking and gizmo for every 2D
type.

In a 2D scene, x and y are the plane, z is only draw order, and rotation is
about z. File > New Scene > 2D makes one with an orthographic camera at
z = 10, looking down -z, which the editor camera starts from. In a 2D scene
the editor camera stays orthographic and looks down -z: the right button
drags the view and the wheel zooms. The gizmo moves and scales in x and y
and rotates about z, the grid lies on z = 0, and there is no orientation
gizmo. Still to come: Add Component menus that offer the 2D components (a
sprite, a 2D camera).

## Values

| Engine type | JSON | Example |
|---|---|---|
| `bool` | boolean | `true` |
| `int` | integer | `64` |
| `float` | number | `0.4` |
| `glm::vec3`, `glm::vec4` | array of 3 or 4 numbers | `[0.0, -0.4, 0.0]` |
| `glm::quat` | array of 4 numbers, **x, y, z, w**, as glTF writes them (glm's constructor takes w first) | `[0.0, 0.0, 0.0, 1.0]` |
| enum | string, the value's key | `"Spot"` |
| mesh, texture | string, a key of `meshes` or `textures`, or `null` | `"Sphere (1, 64, 128)"` |
| material | string, a key of `materials`, or `null` | `"Ground"` |
| entity | string, an entity's `id`, or `null` | `"7efadd669ab6d12c"` |

Values are stored as the engine stores them, not as the editor shows them:

- Angles are radians (the Inspector shows degrees).
- Colors are linear RGB (the Inspector edits sRGB).
- Rotations are quaternions, never Euler angles.

Enums are written by key, so reordering an enum in C++ does not change what
old files mean. A component's enum keys are its property's `enumKeys`
(`"ConvexHull"`, not the Inspector's `"Convex Hull"`).

## Environment

| Key | Value |
|---|---|
| `ambientColor` | vec3, linear RGB, already multiplied by its intensity: the flat ambient term |
| `exposure` | float, applied before tonemapping |

Unity keeps the equivalent in each scene (its `RenderSettings`), Godot in a
`WorldEnvironment` node. Later settings (a skybox, IBL) go here too.

## Meshes and textures

Objects from resource id to how to make the resource. The ids are the
`ResourceManager`'s, so a scene file and a model import share what they
both load. Components and materials reference resources only through these
tables, so the file names each asset once, and a loader knows all it must
load before it creates the first entity (Godot's `ext_resource` table does
the same). Only what the scene uses is written.

```json
"meshes": {
  "Sphere (1, 64, 128)": { "primitive": "Sphere", "radius": 1.0, "stacks": 64, "slices": 128 },
  "Plane (1)": { "primitive": "Plane", "size": 1.0 },
  "Sponza/glTF/Sponza.gltf": { "model": "models/Sponza/glTF/Sponza.gltf" }
},
"textures": {
  "BoxTextured/glTF/BoxTextured.gltf/CesiumLogoFlat.png": {
    "image": "models/BoxTextured/glTF/CesiumLogoFlat.png",
    "colorSpace": "sRGB", "wrapU": "Repeat", "wrapV": "Repeat",
    "magFilter": "Linear", "minFilter": "Nearest", "mipmapMode": "Linear", "mipmaps": true
  },
  "BoxTextured/glTF-Binary/BoxTextured.glb/*0": {
    "model": "models/BoxTextured/glTF-Binary/BoxTextured.glb", "image": "*0",
    "colorSpace": "sRGB", ...
  }
}
```

- **`primitive`**: a mesh from `Resource/Primitives.hpp`, with its arguments
  (`CreateSphere(radius, stacks, slices)`, `CreatePlane(size)`). Its id is
  made of them, so equal primitives share one mesh.
- **`model`** on a mesh: the mesh `ModelImporter` makes of that file: all of
  its meshes baked into one, a submesh per material, in the importer's order.
- **`image`** on a texture: an image file. With `model` as well, it is an
  image embedded in that model file, by the name the file gives it
  (`"*0"`, `"*1"`, ... for glTF, a file name for FBX).
- Texture settings: `colorSpace` (`"sRGB"` or `"Linear"`), `wrapU`, `wrapV`
  (`"Repeat"`, `"MirroredRepeat"`, `"ClampToEdge"`), `magFilter`, `minFilter`
  (`"Linear"`, `"Nearest"`), `mipmapMode` (`"Linear"`, `"Nearest"`), `mipmaps`
  (bool). One left out keeps `TextureSettings`' default.

Paths are relative to the executable's directory, as `FileSystem::getPath()`
resolves them, with forward slashes. A mesh made some other way (built in
code, with no primitive or file behind it) cannot be named, and is saved
as none, with a warning.

## Materials

An object from material id to its values, with `Material`'s fields under
glTF's names:

```json
"Ground": {
  "baseColorFactor": [0.18, 0.18, 0.18, 1.0],
  "metallicFactor": 0.0,
  "roughnessFactor": 0.9,
  "emissiveFactor": [0.0, 0.0, 0.0],
  "emissiveStrength": 1.0,
  "baseColorTexture": "...",
  "normalScale": 1.0,
  "occlusionStrength": 1.0,
  "alphaMode": "Opaque",
  "alphaCutoff": 0.5,
  "doubleSided": false
}
```

The texture keys are `baseColorTexture`, `metallicRoughnessTexture`,
`normalTexture`, `occlusionTexture` and `emissiveTexture`, each left out when
the material has none. `alphaMode` is `"Opaque"`, `"Mask"` or `"Blend"`.

Materials are a table of their own, not part of the mesh components, because
several mesh slots can share one material, and an edit to it changes all of
them. `Material` has no name yet, so the writer names each after the first
entity that uses it (`"Gold 3"`), numbered when taken (`"Sponza 1"`). A
material no entity uses is not saved.

## Entities

```json
{
  "id": "6d3f0e1a9c4b2857",
  "name": "Point Light 0",
  "active": true,
  "components": [ ... ],
  "children": [ ... ]
}
```

| Key | Value |
|---|---|
| `id` | 64 bits as 16 lowercase hex digits, unique in the scene and kept across saves. A string, because many JSON readers hold numbers as doubles, which keep only 53 bits. |
| `name` | The entity's name, unique among its siblings (`Scene::MakeUniqueName`). |
| `active` | The entity's own flag (`Entity::IsActive`). |
| `components` | Its components in order; see below. |
| `children` | Its children in order. Left out when there are none. |

The tree is nested rather than a flat list with parent ids, so a file cannot
name a missing parent or a cycle. `id` is there for references between
entities (the top-level `camera`, later a script's fields), and for prefabs
later. Unity has `fileID` for the same reason.

## Components

An array of objects, in the entity's component order (which is also their
update order). `type` is the component's key (`GNC_COMPONENT_TYPE`); the
other keys are its values. An entity holds at most one component of each
type.

The value keys are the `key`s of the component's properties (`Reflect()`),
so a new component is saved and loaded without code of its own. A property
without a key is not saved: the mesh's per-slot material values are views
of the `materials` table. Version 1's components:

| `type` | Keys |
|---|---|
| `Transform` | `position` vec3, `rotation` quat, `scale` vec3; local to the nearest ancestor with a transform |
| `Mesh` | `mesh` mesh, `materials` array of material ids or `null`, slot i for submesh i |
| `BoundingVolume` | `shape`: `"Box"`, `"Sphere"`, `"Capsule"` or `"ConvexHull"`. The volume itself is refitted on load. |
| `Light` | `lightType` (`"Directional"`, `"Point"`, `"Spot"`), `color` vec3 linear, `intensity` (cd, or lux for directional), `range`, `innerConeAngle`, `outerConeAngle` (radians) |
| `Orbit` | `center` vec3, `radius`, `angularSpeed` (radians per second), `angle` (radians, where it is now) |
| `Camera` | `projection` (`"Perspective"`, `"Orthographic"`), `yfov` (vertical, radians), `ymag` (half the orthographic view's height), `znear`, `zfar` |

`type` is reserved in every component, which is why the light's type is
`lightType`.

## Cameras

A camera is an entity with a `Camera` component, as in Unity and Godot. It
sits at its transform and looks down its local -Z axis, with +Y up, as
lights and glTF cameras do. The camera keys are glTF's, and both projections'
values are kept, so switching the projection back and forth loses nothing.
The aspect ratio is not stored: it comes from the viewport, and the
orthographic width follows from `ymag` and the aspect.

A scene may hold several cameras. The top-level `camera` names the one the
game sees through, so there is exactly one; Unity finds it by the
`MainCamera` tag and Godot by a `current` flag on each camera, which can
leave two or none marked.

The editor's own camera, the one the Viewport flies around with, is not
part of the scene: it moves every time someone looks around, which would
make every save a diff. Unity keeps it in its per-user `Library` folder and
Godot in `.godot/editor`. Here, opening a scene puts the editor camera at
the scene's camera; keeping where it was per user is still to come.

## Loading

1. Read the file. If it is not JSON, `format` is not `"gncscene"`, or
   `version` is newer than the engine reads, refuse it and keep the current
   scene. An older version would be upgraded step by step (1 → 2 → 3) here.
   Then take `name`, `dimension` and `environment`.
2. Load the meshes, then the textures; a model file with embedded images is
   opened once for all of them. One that fails is logged, and what
   references it gets `null` (a mesh draws nothing, a texture slot uses the
   table's default).
3. Create the materials.
4. Create the entities depth first, each after its parent, and give each its
   components in order, setting their values through their properties.
5. Point `camera` at its entity.

Leniency, so files keep working as the engine changes:

- A key the component does not have is logged and skipped. A key it has but
  the file lacks keeps the component's default.
- A value of the wrong type is logged and the default is kept. A reference
  to an id the file does not have is logged and read as `null`.
- A component type the engine does not know is logged, and its JSON is kept
  on the entity, so saving writes it back unchanged (Unity's "missing
  script"), after the components the engine knows.
- A duplicate or malformed entity id gets a new id, with a warning.

Loading replaces the current scene. Additive loading (several scenes at
once, as Unity's `LoadSceneMode.Additive`) can come later.

## Writing

The output depends only on the scene, so saving an unchanged scene changes
no bytes and an edit shows up as a small diff:

- Keys in the order this document lists them; meshes, textures and
  materials in the order the entities first use them.
- Two-space indent. An array of plain values (a vector, a quaternion, a list
  of ids) on one line; everything else one value per line.
- Floats as the shortest text that reads back to the same `float`: `0.4`, not
  `0.4000000059604645`, which is what storing numbers as `double` would
  print. A whole number keeps `.0`.
- Every value is written, defaults too, so changing a default in C++ does
  not change saved scenes. Only absent things are left out: an empty texture
  slot, an empty `children`.
- UTF-8 without a BOM, LF line endings, a final newline.
- Save to `<name>.gncscene.tmp`, then rename it over the file, so a crash
  while saving leaves the old scene intact.
