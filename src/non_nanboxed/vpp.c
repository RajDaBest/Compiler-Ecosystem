#define _VM_IMPLEMENTATION
#define _SV_IMPLEMENTATION

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "./virt_mach.h"
#include "./String_View.h"

#define VM_DEFINE_LIMIT 128
#define MAX_PATH_LENGTH 1024
#define MAX_INCLUDE_FILE_LENGTH 4096
#define MAX_LIB_PATHS 32
#define HASH_TABLE_SIZE 256  // Must be a power of 2

typedef struct vpp_Hashnode {
    String_View label_name;
    String_View label_value;
    struct vpp_Hashnode* next;
} vpp_Hashnode;

typedef struct {
    vpp_Hashnode* buckets[HASH_TABLE_SIZE];
    size_t size;
} HashTable;

typedef struct {
    char paths[MAX_LIB_PATHS][MAX_PATH_LENGTH];
    size_t count;
} LibPaths;

HashTable define_table = {0};
bool preprocessing_failed = false;

uint32_t vpp_hash_sv(String_View sv) {
    uint32_t hash = 2166136261u;  
    for (size_t i = 0; i < sv.count; i++) {
        hash ^= (uint8_t)sv.data[i];
        hash *= 16777619u;  
    }
    return hash;
}

// Initialize a new hash node
vpp_Hashnode* create_hash_node(String_View name, String_View value) {
    vpp_Hashnode* node = malloc(sizeof(vpp_Hashnode));
    if (!node) return NULL;
    
    node->label_name = name;
    node->label_value = value;
    node->next = NULL;
    return node;
}

// Insert or update a define in the hash table
bool hash_table_insert(String_View name, String_View value) {
    uint32_t hash = vpp_hash_sv(name);
    size_t index = hash & (HASH_TABLE_SIZE - 1);
    
    vpp_Hashnode* current = define_table.buckets[index];
    
    // Check if the label already exists
    while (current != NULL) {
        if (sv_eq(current->label_name, name)) {
            current->label_value = value;  // Update existing value
            return true;
        }
        current = current->next;
    }
    
    // Create new node
    vpp_Hashnode* new_node = create_hash_node(name, value);
    if (!new_node) return false;
    
    // Insert at the beginning of the bucket
    new_node->next = define_table.buckets[index];
    define_table.buckets[index] = new_node;
    define_table.size++;
    
    return true;
}

// Look up a define in the hash table
vpp_Hashnode* hash_table_lookup(String_View name) {
    uint32_t hash = vpp_hash_sv(name);
    size_t index = hash & (HASH_TABLE_SIZE - 1);
    
    vpp_Hashnode* current = define_table.buckets[index];
    while (current != NULL) {
        if (sv_eq(current->label_name, name)) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

// Clean up the hash table
void hash_table_cleanup(void) {
    for (size_t i = 0; i < HASH_TABLE_SIZE; i++) {
        vpp_Hashnode* current = define_table.buckets[i];
        while (current != NULL) {
            vpp_Hashnode* next = current->next;
            free(current);
            current = next;
        }
        define_table.buckets[i] = NULL;
    }
    define_table.size = 0;
}

void split_env_paths(const char* env_value, LibPaths* lib_paths) {
#ifdef _WIN32
    const char PATH_SEPARATOR = ';';
#else
    const char PATH_SEPARATOR = ':';
#endif

    if (!env_value) return;

    char env_copy[MAX_PATH_LENGTH];
    strncpy(env_copy, env_value, MAX_PATH_LENGTH - 1);
    env_copy[MAX_PATH_LENGTH - 1] = '\0';

    char* token = strtok(env_copy, &PATH_SEPARATOR);
    while (token && lib_paths->count < MAX_LIB_PATHS) {
        strncpy(lib_paths->paths[lib_paths->count], token, MAX_PATH_LENGTH - 1);
        lib_paths->paths[lib_paths->count][MAX_PATH_LENGTH - 1] = '\0';
        lib_paths->count++;
        token = strtok(NULL, &PATH_SEPARATOR);
    }
}

FILE* try_open_include_file(const char* filename, const LibPaths* lib_paths, bool is_system_include) {
    FILE* file = NULL;
    char filepath[MAX_PATH_LENGTH];

    if (!is_system_include) {
        // For "", try current directory first
        file = fopen(filename, "r");
        if (file) return file;
    }

    // Try all library paths
    for (size_t i = 0; i < lib_paths->count; i++) {
        snprintf(filepath, sizeof(filepath), "%s%c%s", // a very useful function for string manipulation along with fprintf
                 lib_paths->paths[i],
#ifdef _WIN32
                 '\\',
#else
                 '/',
#endif
                 filename);
        file = fopen(filepath, "r");
        if (file) return file;
    }

    return NULL;
}

void print_usage_and_exit(void) {
    fprintf(stderr, "Usage: ./program [--lib <library_path>]... [--vlib-ignore] <input_file> [output_file]\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage_and_exit();
    }

    LibPaths lib_paths = {0};
    char *input = NULL;
    char *output = NULL;
    bool vlib_ignore = false;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--lib") == 0) {
            if (i + 1 < argc && lib_paths.count < MAX_LIB_PATHS) {
                strncpy(lib_paths.paths[lib_paths.count], argv[i + 1], MAX_PATH_LENGTH - 1);
                lib_paths.paths[lib_paths.count][MAX_PATH_LENGTH - 1] = '\0';
                lib_paths.count++;
                i++;
            } else {
                fprintf(stderr, "ERROR: Invalid --lib usage\n");
                print_usage_and_exit();
            }
        } else if (strcmp(argv[i], "--vlib-ignore") == 0) {
            vlib_ignore = true;
        } else if (!input) {
            input = argv[i];
        } else if (!output) {
            output = argv[i];
        }
    }

    if (!input) {
        fprintf(stderr, "ERROR: Missing input file\n");
        print_usage_and_exit();
    }

    // Handle VLIB environment variable if not ignored
    if (!vlib_ignore) {
        LibPaths env_paths = {0};
        split_env_paths(getenv("VLIB"), &env_paths);
        
        // Shift existing paths to make room for env paths at the beginning
        if (env_paths.count > 0) {
            size_t total_paths = env_paths.count + lib_paths.count;
            if (total_paths > MAX_LIB_PATHS) {
                total_paths = MAX_LIB_PATHS;
            }
            
            memmove(&lib_paths.paths[env_paths.count], 
                    lib_paths.paths, 
                    (total_paths - env_paths.count) * MAX_PATH_LENGTH); // make space in from of lib_paths.paths for the environment paths
            
            memcpy(lib_paths.paths, env_paths.paths, 
                   env_paths.count * MAX_PATH_LENGTH);
            
            lib_paths.count = total_paths;
        }
    }

    // Create default output filename if not provided
    if (!output) {
        output = malloc(MAX_PATH_LENGTH);
        snprintf(output, MAX_PATH_LENGTH, "%.*s.vpp", 
                 (int)(strrchr(input, '.') - input), input);
    }

    while (true) {
        String_View include_processing = slurp_file(input);
        String_View copy_one = include_processing;
        size_t line_no = 0;

        if (include_processing.data == NULL) {
            fprintf(stderr, "ERROR: Failed to read input file: %s\n", 
                    strerror(errno));
            exit(EXIT_FAILURE);
        }

        FILE *temp = fopen("temp", "w");
        if (!temp) {
            fprintf(stderr, "ERROR: Failed to create temp file: %s\n", 
                    strerror(errno));
            exit(EXIT_FAILURE);
        }

        size_t include_no = 0;
        while (include_processing.count > 0) {
            String_View line = sv_chop_by_delim(&include_processing, '\n');

            if (*line.data == '%') {
                String_View directive = sv_chop_by_delim(&line, ' ');
                sv_trim_left(&line);

                if (sv_eq(directive, cstr_as_sv("%include"))) {
                    if (line.count == 0) {
                        fprintf(stderr, "Line Number %zu -> ERROR: %%include directive has no file\n", 
                                line_no);
                        preprocessing_failed = true;
                        continue;
                    }

                    bool is_system_include = false;
                    String_View include_label = line;
                    
                    // Determine include type and extract filename
                    if (line.data[0] == '<') {
                        is_system_include = true;
                        include_label = sv_chop_by_delim(&line, '>');
                        sv_chop_by_delim(&include_label, '<');
                    } else if (line.data[0] == '"') {
                        line.data++; line.count--;
                        include_label = sv_chop_by_delim(&line, '"');
                    }

                    if (!include_label.count) {
                        fprintf(stderr, "Line number %zu -> ERROR: Invalid %%include usage\n", 
                                line_no);
                        preprocessing_failed = true;
                        continue;
                    }

                    char file_name[MAX_PATH_LENGTH];
                    snprintf(file_name, sizeof(file_name), "%.*s", 
                             (int)include_label.count, include_label.data);

                    FILE *file = try_open_include_file(file_name, &lib_paths, is_system_include);
                    if (!file) {
                        fprintf(stderr, "ERROR: Failed to open include file %s\n", file_name);
                        preprocessing_failed = true;
                        continue;
                    }

                    fseek(file, 0, SEEK_END);
                    size_t file_size = ftell(file);
                    fseek(file, 0, SEEK_SET);

                    if (file_size > MAX_INCLUDE_FILE_LENGTH) {
                        fprintf(stderr, "Line Number %zu -> ERROR: %%include exceeded MAX_INCLUDE_FILE_LENGTH\n", 
                                line_no);
                        fclose(file);
                        continue;
                    }

                    char *buffer = malloc(file_size + 1);
                    if (!buffer) {
                        fprintf(stderr, "ERROR: Failed to allocate memory for include file\n");
                        fclose(file);
                        preprocessing_failed = true;
                        continue;
                    }

                    fread(buffer, 1, file_size, file);
                    fwrite(buffer, 1, file_size, temp);
                    fprintf(temp, "\n");

                    free(buffer);
                    fclose(file);
                    include_no++;
                } else {
                    fprintf(temp, "%.*s %.*s\n", (int)directive.count, 
                            directive.data, (int)line.count, line.data);
                }
            } else {
                fprintf(temp, "%.*s\n", (int)line.count, line.data);
            }
            line_no++;
        }

        free((void *)copy_one.data);
        fclose(temp);
        if (!include_no) break;

        input = "temp";
    }

    // Process defines using hash table
    FILE *vpp = fopen(output, "w");
    if (!vpp) {
        fprintf(stderr, "ERROR: Failed to open output file %s: %s\n", 
                output, strerror(errno));
        exit(EXIT_FAILURE);
    }

    String_View unprocessed_file = slurp_file("temp");
    String_View copy_two = unprocessed_file;

    size_t line_no = 0;
    while (unprocessed_file.count > 0) {
        String_View line = sv_chop_by_delim(&unprocessed_file, '\n');
        sv_trim_left(&line);
        sv_trim_right(&line);

        if (*line.data == '%') {
            String_View directive = sv_chop_by_delim(&line, ' ');
            sv_trim_left(&line);

            if (sv_eq(directive, cstr_as_sv("%define"))) {
                if (line.count == 0) {
                    fprintf(stderr, "Line Number %zu -> ERROR: %%define directive has no label\n", 
                            line_no);
                    preprocessing_failed = true;
                    continue;
                }

                String_View define_label = sv_chop_by_delim(&line, ' ');
                String_View label_value = sv_chop_by_delim(&line, ' ');

                if (!hash_table_insert(define_label, label_value)) {
                    fprintf(stderr, "ERROR: Failed to insert define into hash table\n");
                    preprocessing_failed = true;
                }
            }
        } else {
            while (line.count > 0) {
                String_View token = sv_chop_by_delim(&line, ' ');
                vpp_Hashnode* node = hash_table_lookup(token);
                
                if (node && node->label_value.count > 0) {
                    fprintf(vpp, "%.*s ", (int)node->label_value.count, 
                            node->label_value.data);
                } else {
                    fprintf(vpp, "%.*s ", (int)token.count, token.data);
                }
            }
            fprintf(vpp, "\n");
        }
        line_no++;
    }

    // Cleanup
    hash_table_cleanup();
    free((void *)copy_two.data);
    if (output != argv[argc-1]) free(output);
    fclose(vpp);
    remove("temp");

    return preprocessing_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}