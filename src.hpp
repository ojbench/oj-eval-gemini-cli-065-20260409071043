#ifndef SRC_HPP
#define SRC_HPP

#include <cstddef>
#include <list>
#include <map>
#include <vector>
#include <iostream>

int* getNewBlock(int n);
void freeBlock(const int* block, int n);

class Allocator {
private:
    struct Block {
        int* ptr;
        int n; // size in ints (n * 1024 ints = n * 4096 bytes)
        int used; // number of ints used
        std::list<std::pair<int, int>> free_chunks; // offset, size in ints
        
        Block(int* p, int size) : ptr(p), n(size), used(0) {
            free_chunks.push_back({0, size * 1024});
        }
    };
    
    std::list<Block> blocks;

public:
    Allocator() {}

    ~Allocator() {
        for (auto& block : blocks) {
            freeBlock(block.ptr, block.n);
        }
    }

    int* allocate(int n) {
        // Try to find a free chunk in existing blocks
        for (auto& block : blocks) {
            for (auto it = block.free_chunks.begin(); it != block.free_chunks.end(); ++it) {
                if (it->second >= n) {
                    int* res = block.ptr + it->first;
                    if (it->second == n) {
                        block.free_chunks.erase(it);
                    } else {
                        it->first += n;
                        it->second -= n;
                    }
                    block.used += n;
                    return res;
                }
            }
        }
        
        // Allocate a new block
        int block_n = (n + 1023) / 1024;
        int* new_ptr = getNewBlock(block_n);
        blocks.emplace_back(new_ptr, block_n);
        auto& block = blocks.back();
        
        int* res = block.ptr;
        if (block_n * 1024 == n) {
            block.free_chunks.clear();
        } else {
            block.free_chunks.front().first += n;
            block.free_chunks.front().second -= n;
        }
        block.used += n;
        return res;
    }

    void deallocate(int* pointer, int n) {
        for (auto it = blocks.begin(); it != blocks.end(); ++it) {
            auto& block = *it;
            if (pointer >= block.ptr && pointer < block.ptr + block.n * 1024) {
                int offset = pointer - block.ptr;
                
                // Insert the free chunk and merge if possible
                auto chunk_it = block.free_chunks.begin();
                while (chunk_it != block.free_chunks.end() && chunk_it->first < offset) {
                    ++chunk_it;
                }
                
                auto new_chunk = block.free_chunks.insert(chunk_it, {offset, n});
                
                // Merge with next
                auto next_chunk = new_chunk;
                ++next_chunk;
                if (next_chunk != block.free_chunks.end() && new_chunk->first + new_chunk->second == next_chunk->first) {
                    new_chunk->second += next_chunk->second;
                    block.free_chunks.erase(next_chunk);
                }
                
                // Merge with prev
                if (new_chunk != block.free_chunks.begin()) {
                    auto prev_chunk = new_chunk;
                    --prev_chunk;
                    if (prev_chunk->first + prev_chunk->second == new_chunk->first) {
                        prev_chunk->second += new_chunk->second;
                        block.free_chunks.erase(new_chunk);
                    }
                }
                
                block.used -= n;
                
                // If block is completely free, release it
                if (block.used == 0) {
                    freeBlock(block.ptr, block.n);
                    blocks.erase(it);
                }
                return;
            }
        }
    }
};

#endif
