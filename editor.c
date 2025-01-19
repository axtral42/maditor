#include "./editor.h"
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>

#define LINE_INIT_CAPACITY 1024
#define EDITOR_INIT_CAPACITY 128
#define CHUNK_INIT_CAPACITY 1024



static void line_grow(Line* line, size_t n){

    size_t new_capacity= line->capacity;
    assert(new_capacity >= line->size);
    while (new_capacity < line->size + n){
        if (new_capacity==0){
            new_capacity = LINE_INIT_CAPACITY;
        }
        else{
            new_capacity*=2;
        }
    }
    if (new_capacity != line->capacity){
        line->chars= realloc(line->chars, new_capacity);
        line->capacity = new_capacity;
    }

}


void line_insert_sized_before(Line *line, const char *text, size_t *col, size_t text_size){
    if (*col > line->size){
        *col=line->size;
    }

    line_grow(line,text_size);
    memmove(line->chars+ *col+text_size,line->chars+*col,line->size-*col);
    memcpy(line->chars + *col, text, text_size);
    line->size += text_size;
    *col += text_size;
}

void line_insert_before(Line *line, const char *text, size_t *col){
         size_t text_size = strlen(text);
    line_insert_sized_before(line, text, col, text_size);
}

void line_append_sized(Line *line, const char *text, size_t text_size){
    size_t col= line->size;
    line_insert_sized_before(line, text, &col, text_size);
}

void line_append(Line *line, const char *text){
    size_t text_size = strlen(text);
    line_append_sized(line, text, text_size);
}

void line_backspace(Line *line, size_t *col){
    if (*col > line->size){
        *col=line->size;
    }
     if (line->size > 0 && *col>0){
            memmove(line->chars+*col -1, line->chars+*col, line->size - *col);
            line->size -= 1;
            *col-=1;
     }
}
void line_delete(Line *line, size_t *col){
    if (*col > line->size){
        *col=line->size;
    }
     if (line->size > 0 && *col<line->size){
            memmove(line->chars+*col, line->chars+*col +1 , line->size - *col);
            line->size -= 1;
     }
}

static void editor_grow(Editor* editor, size_t n){

    size_t new_capacity= editor->capacity;
    assert(new_capacity >= editor->size);
    while (new_capacity < editor->size + n){
        if (new_capacity==0){
            new_capacity = EDITOR_INIT_CAPACITY;
        }
        else{
            new_capacity*=2;
        }
    }
    if (new_capacity != editor->capacity){
        editor->lines= realloc(editor->lines, new_capacity * sizeof(editor->lines[0]));
        editor->capacity = new_capacity;
    }

}

void editor_push_new_line(Editor* editor){
    editor_grow(editor, 1);
    memset(&editor->lines[editor->size], 0, sizeof(editor->lines[0]));
    editor->size +=1;
    
}

void editor_check_line(Editor* editor){
    if (editor->cursor_row >= editor->size){
        if (editor->size > 0){
            editor->cursor_row = editor->size -1;
        }
        else{
            editor_push_new_line(editor);
        }
    }
}


void editor_insert_new_line(Editor* editor){
     editor_check_line(editor);

     size_t line_size = sizeof(editor->lines[0]);
    editor_grow(editor,1);
    memmove(editor->lines+ (editor->cursor_row +1),editor->lines + (editor->cursor_row) ,(editor->size - editor->cursor_row) * line_size); //[UNDERSTAND} no need to multiply by line size, causes crash on inserting between text
    editor->cursor_row+=1;
    memset(&editor->lines[editor->cursor_row], 0, line_size);
    editor->size += 1;
    editor->cursor_col=0;
}

void editor_insert_before_cursor(Editor* editor, const char* text){
    editor_check_line(editor);

    line_insert_before(&editor->lines[editor->cursor_row], text, &editor->cursor_col);
}
void editor_backspace(Editor* editor){
    editor_check_line(editor);

    line_backspace(&editor->lines[editor->cursor_row], &editor->cursor_col);
}
void editor_delete(Editor* editor){
    editor_check_line(editor);

    line_delete(&editor->lines[editor->cursor_row],&editor->cursor_col);
}

const char* editor_char_under_cursor(Editor* editor){
    if (editor->size == 0){
            editor_push_new_line(editor);
    }
    if (editor->cursor_row < editor->size){
        if (editor->cursor_col < editor->lines[editor->cursor_row].size){
            return &editor->lines[editor->cursor_row].chars[editor->cursor_col];
        }
    }
    return NULL;

}

void editor_save_to_file(const Editor *editor, const char *file_path){
    FILE *f=fopen(file_path, "w");
    if (f ==NULL){
        fprintf(stdout, "ERROR: Couldn't open file %s because of %s\n",file_path,strerror(errno));
    }

    for (size_t row=0; row< editor->size; row++){
        fwrite(editor->lines[row].chars, 1, editor->lines[row].size,f);
        fputc('\n',f);
    }
    fclose(f);
    fprintf(stdout, "SUCCESS: Content saved to %s\n",file_path);
}

void editor_load_from_file( Editor *editor, const char *file_path){
    assert(editor->lines == NULL && "The editor should be empty");

    FILE *f = fopen(file_path, "r");

    if (!f){
        fprintf(stderr, "ERROR: couldn't open %s because of %s", file_path, strerror(errno));
        exit(1);
    }

    static char chunk[CHUNK_INIT_CAPACITY];
    

    while(!feof(f)){
        size_t n = fread(chunk, 1, sizeof(chunk), f);
        size_t count=0;
        char append[CHUNK_INIT_CAPACITY +1];
        memset(append, 0, sizeof(append));
        //fwrite(chunk, 1, n, stdout);
        for (size_t i=0; i<n; i++){
            if (chunk[i]=='\n'){
                count=0; 
                editor_insert_before_cursor(editor, append);
                editor_insert_new_line(editor);
                memset(append, 0, sizeof(append));
            }
            else{
            append[count++]=chunk[i];
            }
        }
        editor_insert_before_cursor(editor, append); // final append to add caracters in the last line which may not have been picked up due to lack of \n

    } 

    fclose(f);
}