#include <stdlib.h>

#ifndef EDITOR_H_
#define EDITOR_H_

typedef struct{
    size_t capacity;
    size_t size;
    char* chars;
} Line;

void line_insert_before(Line *line, const char *text, size_t *col);
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



#endif //EDITOR_H_ 