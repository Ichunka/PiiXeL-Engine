# How to Manage Assets

## Supported Asset Types

```
.png, .jpg       → Textures
.spritesheet     → Sprite sheets
.animclip        → Animation clips
.animcontroller  → Animator controllers
.wav, .ogg, .mp3 → Audio
.scene           → Scenes
```

## Asset System Architecture

```
Asset File (.png)
    ↓
Asset Metadata (.pxa)     ← UUID, type, dependencies
    ↓
AssetRegistry             ← Runtime lookup by UUID
    ↓
Your Code                 ← Access via UUID
```

## Importing Assets

### Automatic Import

1. Copy file to `content/` directory
2. Editor automatically scans on startup
3. `.pxa` file created next to asset
4. UUID assigned and cached

### Manual Import

**Editor → Content Browser → Right Click → Import Asset**

## Using Assets in Scripts

### Load by UUID
```cpp
class MyScript : public PiiXeL::ScriptComponent {
public:
    PiiXeL::UUID textureAsset{0};  // Exposed in inspector

private:
    Texture2D m_Texture;

    void OnStart() override {
        // Load asset by UUID
        std::shared_ptr<PiiXeL::Asset> asset =
            PiiXeL::AssetRegistry::Instance().GetAsset(textureAsset);

        if (asset && asset->GetMetadata().type == PiiXeL::AssetType::Texture) {
            PiiXeL::TextureAsset* texAsset =
                dynamic_cast<PiiXeL::TextureAsset*>(asset.get());
            if (texAsset) {
                m_Texture = texAsset->GetTexture();
            }
        }
    }
};
```

### Sprite Component (Automatic)
```cpp
// Sprite component handles texture loading automatically
// Just drag texture onto Sprite in inspector
```

## Asset Registry API

```cpp
// Get asset by UUID
std::shared_ptr<Asset> asset = AssetRegistry::Instance().GetAsset(uuid);

// Get UUID from path
UUID uuid = AssetRegistry::Instance().GetUUIDFromPath("content/player.png");

// Load asset from path
std::shared_ptr<Asset> asset =
    AssetRegistry::Instance().LoadAssetFromPath("content/player.png");

// Check if asset is loaded
bool loaded = AssetRegistry::Instance().IsAssetLoaded(uuid);
```

## Asset Metadata (.pxa)

Automatically generated JSON file:

```json
{
  "uuid": 12345678901234567890,
  "type": "texture",
  "path": "content/player.png",
  "name": "player",
  "dependencies": []
}
```

**DO NOT** manually edit `.pxa` files!

## Asset Organization

```
content/
  ├── textures/
  │   ├── player.png
  │   ├── player.png.pxa
  │   └── enemies/
  │       ├── zombie.png
  │       └── zombie.png.pxa
  ├── animations/
  │   ├── player_idle.animclip
  │   ├── player_walk.animclip
  │   └── player.animcontroller
  ├── audio/
  │   ├── jump.wav
  │   └── music.ogg
  └── scenes/
      ├── level1.scene
      └── menu.scene
```

## Drag & Drop

- **Textures** → Drag onto Sprite component
- **Animator Controllers** → Drag onto Animator component
- **Scenes** → Double-click to load

## Asset Browser Features

- **Search**: Type to filter assets
- **Folders**: Organize assets hierarchically
- **Rename**: F2 or right-click
- **Delete**: Del or right-click → Delete
- **New Asset**: Right-click → New → [Type]

## Asset UUIDs

- **Persistent** across moves/renames
- **64-bit** unique identifiers
- **References** update automatically
- **Cached** in `.asset_uuid_cache` (gitignored)

## Git Workflow

**Commit:**
- `.pxa` files ✓
- Source assets (.png, .wav, etc.) ✓

**Ignore:**
- `datas/.asset_uuid_cache` ✗ (regenerated on startup)

## Asset Loading Performance

- **Lazy loading**: Assets loaded on first use
- **Caching**: Loaded assets stay in memory
- **Shared**: Multiple components can share same asset

## Common Asset Workflows

### Add Character Texture
1. Copy `player.png` to `content/textures/`
2. Editor auto-imports on next launch
3. Drag onto Sprite component in inspector

### Create Animated Character
1. Import texture
2. Create sprite sheet (define frames)
3. Create animation clips (idle, walk, jump)
4. Create animator controller (state machine)
5. Add Animator component to entity
6. Assign controller

### Add Background Music
1. Copy `music.ogg` to `content/audio/`
2. Editor auto-imports
3. Use audio API to play (TODO: Document audio API)

## Troubleshooting

**Asset not found:**
- Check `.pxa` file exists
- Verify UUID in inspector matches `.pxa`
- Restart editor to rebuild cache

**Wrong asset loaded:**
- UUID collision (rare)
- Delete `.asset_uuid_cache` and restart

**Asset not importing:**
- Check file extension is supported
- Ensure file is in `content/` directory
- Check editor console for errors
