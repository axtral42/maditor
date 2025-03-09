#include <stdio.h>
#include <stdlib.h>

#ifndef EDITOR_H_
#define EDITOR_H_

typedef struct{
    size_t capacity;
    size_t size;
    char* chars;
} Line;

void line_insert_sized_before(Line *line, const char *text, size_t *col, size_t text_size);
void line_insert_before(Line *line, const char *text, size_t *col);
void line_append_sized(Line *line, const char *text, size_t text_size);
void line_append(Line *line, const char *text);
void line_backspace(Line *line, size_t *col);
void line_delete(Line *line, size_t *col);

typedef struct {
    size_t capacity;
    size_t size;
    size_t cursor_row;
    size_t cursor_col;
    Line* lines;

} Editor;

void editor_insert_before_cursor(Editor* editor, const char* text);
void editor_insert_new_line(Editor* editor);
void editor_backspace(Editor* editor);
void editor_delete(Editor* editor);
const char* editor_char_under_cursor(Editor* editor);
void editor_save_to_file( Editor *editor, const char *file_path);
void editor_load_from_file(Editor *editor, FILE *f);

#endif //EDITOR_H_ 
