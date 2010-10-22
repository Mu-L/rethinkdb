
#ifndef __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__
#define __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__

#include "containers/segmented_vector.hpp"
#include "containers/intrusive_list.hpp"
#include "disk_structure.hpp"
#include "config/alloc.hpp"

struct in_memory_index_t :
    public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, in_memory_index_t>
{
    segmented_vector_t<flagged_off64_t, MAX_BLOCK_ID> blocks;

public:
    in_memory_index_t() {
        /* This constructor is used when we are creating a new database. Initially no blocks
        exist. */
    }
    
    in_memory_index_t(lba_disk_structure_t **shards, int n_shards, extent_manager_t *em) {
        
        for (int i = 0; i < n_shards; i++) {
            
            lba_disk_structure_t *s = shards[i];
            
            /* Call each lba_extent_t in the same order they were written */
            
            if (s->superblock) {
                for (lba_disk_extent_t *e = s->superblock->extents.head();
                     e;
                     e = s->superblock->extents.next(e)) {
                    fill_from_extent(e);
                }
            }
            if (s->last_extent) fill_from_extent(s->last_extent);
        }
    }

public:
    void fill_from_extent(lba_disk_extent_t *x) {
    
        for (int i = 0; i < x->count; i ++) {
            
            lba_entry_t *entry = &x->data->data()->entries[i];
        
            // Skip it if it's padding
            if (lba_entry_t::is_padding(entry))
                continue;
            assert(entry->block_id != PADDING_BLOCK_ID);
        
            // Sanity check. If this assertion fails, it probably means that the file was
            // corrupted, or was created with a different device block size.
            assert(entry->offset.parts.value % DEVICE_BLOCK_SIZE == 0);

            set_block_offset(entry->block_id, entry->offset);
        }
    }
    
    ser_block_id_t max_block_id() {
        return blocks.get_size();
    }
    
    flagged_off64_t get_block_offset(ser_block_id_t id) {
        if(id >= blocks.get_size()) {
            return flagged_off64_t::unused();
        } else {
            return blocks[id];
        }
    }
    
    void set_block_offset(ser_block_id_t id, flagged_off64_t offset) {
        
        /* Grow the array if necessary, and fill in the empty space with flagged_off64_t::unused(). */
        if (id >= blocks.get_size()) {
            blocks.set_size(id + 1, flagged_off64_t::unused());
        }
        
        blocks[id] = offset;
    }
    
    void print() {
#ifndef NDEBUG
        printf("LBA:\n");
        for (unsigned int i = 0; i < blocks.get_size(); i++) {
            printf("%d %.8lx\n", i, (unsigned long int)blocks[i].whole_value);
        }
#endif
    }
};

#endif /* __SERIALIZER_LOG_LBA_IN_MEMORY_INDEX__ */

