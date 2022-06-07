#pragma once
#include <vector>

#include "tile.h"
#include "../utils/math.h"

#pragma pack(push, 1)
struct WorldTileMap {
    utils::math::Vec2<int32_t> size;
    uint32_t count;
    std::vector<Tile*> tiles;

    WorldTileMap() : size(), count(0) {}
    ~WorldTileMap()
    {
        for (auto& tile : tiles)
            delete tile;

        tiles.clear();
    }

    void serialize(void* buffer, std::size_t& position, uint16_t version)
    {
        BinaryReader br{ buffer };
        br.skip(position);

        size = br.read<utils::math::Vec2<int32_t>>();
        count = br.read<uint32_t>();

        tiles.reserve(count);

        position = br.position();
        for (uint32_t i = 0; i < count; i++) {
            Tile tile{};
            tile.serialize(buffer, position, version);
            tiles.push_back(new Tile{ tile });
        }
    }
};
#pragma pack(pop)
