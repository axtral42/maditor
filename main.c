#include<stdio.h>
#include<stdlib.h>
#include<SDL.h>
#include<stdbool.h>
#include <math.h>
#include <time.h>
#include<unistd.h>

#include "./la.h"

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

#define FONT_COLS 18
#define FONT_ROWS 7
#define FONT_WIDTH 128
#define FONT_HEIGHT 64
#define FONT_CHAR_WIDTH (FONT_WIDTH / FONT_COLS)
#define FONT_CHAR_HEIGHT (FONT_HEIGHT / FONT_ROWS)
#define FONT_SCALE 5

#define ASCII_DISPLAY_LOW 32
#define ASCII_DISPLAY_HIGH 126

#define BUFFER_CAPACITY 1024
#define FONT_COLOR 0xFFFFFFFF

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define UNHEXRGBA(color) \
(color >> (8 * 0)) & 0xff, \
(color >> (8 * 1)) & 0xff, \
(color >> (8 * 2)) & 0xff, \
(color >> (8 * 3)) & 0xff

#define UNHEXRGB(color) \
(color >> (8 * 0)) & 0xff, \
(color >> (8 * 1)) & 0xff, \
(color >> (8 * 2)) & 0xff

void scc (int code) { 
    //sdl check code, checks output code of sdl functions, if error, writes that to stderr
    if (code<0){
        fprintf(stderr, "SDL Error: %s\n", SDL_GetError()); //fprintf is used to write to files, here we're writing to stderr file stream the sdl error to nicely handle error
        exit(1);
    }
}

void* scp(void* ptr){
    //sdl check pointer, checks the returned pointer from sdl functions, if null, the function failed and hence catch error
    if (ptr==NULL){
        fprintf(stderr, "SDL Error: %s\n", SDL_GetError()); //fprintf is used to write to files, here we're writing to stderr file stream the sdl error to nicely handle error
        exit(1);
    }
    return ptr; //return the original pointer
}

SDL_Surface* surface_from_file(const char *file_path){
    int width,height,n;
    unsigned char *pixels= stbi_load(file_path, &width, &height, &n,STBI_rgb_alpha);
    if (pixels==NULL){
        fprintf(stderr,"Could not load file %s: %s\n",file_path,stbi_failure_reason());
        exit(1);
    }

    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        const Uint32 rmask = 0xff000000;
        const Uint32 gmask = 0x00ff0000;
        const Uint32 bmask = 0x0000ff00;
        const Uint32 amask = 0x000000ff;
    #else //little endian like x86
        const Uint32 rmask = 0x000000ff;
        const Uint32 gmask = 0x0000ff00;
        const Uint32 bmask = 0x00ff0000;
        const Uint32 amask = 0xff000000;
    #endif

    const int depth =32;
    const int pitch = 4* width;

    return scp(SDL_CreateRGBSurfaceFrom((void*) pixels, width, height, depth, pitch, rmask, gmask, bmask, amask));
}

typedef struct{
    SDL_Texture* spritesheet;
    SDL_Rect glyph_table [ASCII_DISPLAY_HIGH - ASCII_DISPLAY_LOW + 1]; 
} Font;

Font font_load_from_file(SDL_Renderer* renderer, const char *file_path){
    Font font={0};
     SDL_Surface* font_surface= surface_from_file(file_path);
     font.spritesheet =scp(SDL_CreateTextureFromSurface(renderer,font_surface));
     SDL_FreeSurface(font_surface);

     for(size_t ascii = ASCII_DISPLAY_LOW; ascii <= ASCII_DISPLAY_HIGH; ++ascii){ //precalculates glyphh/size for all font characters
         const size_t index = ascii - ASCII_DISPLAY_LOW; 
         const size_t col = index % FONT_COLS;
         const size_t row = index / FONT_COLS;

         font.glyph_table[index] =(SDL_Rect) {
        .x=col*FONT_CHAR_WIDTH,
        .y=row*FONT_CHAR_HEIGHT,
        .w=FONT_CHAR_WIDTH,
        .h=FONT_CHAR_HEIGHT
        };
     }
    return font;
}



void render_char(SDL_Renderer* renderer, Font* font,char c,  Vec2f pos, Uint32 color, float scale, Uint32 r, int mode){
    int c_val=c;
    const SDL_Rect dst={
        .x=(int) floorf(pos.x),
        .y=(int) floorf(pos.y),
        .w=(int) floorf(FONT_CHAR_WIDTH * scale),
        .h=(int) floorf(FONT_CHAR_HEIGHT * scale)
    };
    assert(c>= ASCII_DISPLAY_LOW);
    assert(c<=ASCII_DISPLAY_HIGH);

    const size_t index = c - ASCII_DISPLAY_LOW;

    
    Uint32 c2=pow(c_val,5);
    if (mode==0){
        r=rand(); //rand works but rand_r with seed doesn't work, why?
    }
    else if (mode==1){
        c_val |= 0xffffff; 
        c2=0;
        r=0;
    }
    //removing c_val makes all the text black, understand why
    scc(SDL_SetTextureColorMod(font->spritesheet, (color & (c_val^ c2 ^ r) >> (8 * 0)) & 0xff,(color & (c_val^ c2 ^ r) >> (8 * 1)) & 0xff,(color & (c_val^ c2 ^ r) >> (8 * 2)) & 0xff));
    scc(SDL_SetTextureAlphaMod(font->spritesheet, (color >> (8 * 3)) & 0xff)); //sets the color and alpha value for the given input string of text
    scc(SDL_RenderCopy(renderer,font->spritesheet,&font->glyph_table[index],&dst)); //moved to the last so updated texture mod is reflected

}

void render_text_sized(SDL_Renderer* renderer, Font* font,const char* text, size_t text_size,  Vec2f pos, Uint32 color, float scale, Uint32 r, int mode){
    
    
    Vec2f pen= pos;

    for(size_t i=0; i<text_size; ++i){
        render_char(renderer,font, text[i],  pen, color, scale, r, mode);
        pen.x += FONT_CHAR_WIDTH * scale;
    }
}

void render_text(SDL_Renderer* renderer, Font* font,const char* text,  Vec2f pos, Uint32 color, float scale, Uint32 r, int mode){
    render_text_sized(renderer,font,text, strlen(text), pos,color, scale, r, mode);
}

char buffer[BUFFER_CAPACITY];
size_t buffer_size=0;
size_t buffer_cursor=0;

void render_cursor(SDL_Renderer* renderer, Font* font, Uint32 color){
    const SDL_Rect rect={
        .x=(int) floorf(buffer_cursor*FONT_CHAR_WIDTH*FONT_SCALE),
        .y=0 ,
        .w=FONT_CHAR_WIDTH * FONT_SCALE,
        .h=FONT_CHAR_HEIGHT * FONT_SCALE
    };
    //scc(SDL_RenderCopy(renderer,font->spritesheet,&font->glyph_table[92],&rect));  //use a | as a cursor, will also change color with the text, no blinking
    scc(SDL_SetRenderDrawColor(renderer, UNHEXRGBA(color))); //try to understand how this function works
    scc(SDL_RenderFillRect(renderer, &rect));
}

void render_char_len(SDL_Renderer* renderer, Font* font, Uint32 color, float scale){
    char schars[100]= "characters:";
    char len[10];
    sprintf(len, "%lu", buffer_size);
    strcat(schars,len);
    //printf("%s\n",schars);
    //for some reason the first letter is synced in color with entered text instead of the first character of entered text, check why
    //reason: in the render char function render copy copied the texture to buffer before texture mod was set, hence color mismatch happened, as c was copied before setting to white
    //sol: moved render copy to the end of function and after texture mod set
    render_text(renderer, font, schars, vec2f(0.0, WINDOW_HEIGHT-(FONT_CHAR_HEIGHT*scale)), color, scale, 0, 1);
}

int main (void){
    // Taking current time as seed
    unsigned int seed = time(0);
    Uint32 r=rand_r(&seed); //random seed
    scc(SDL_Init(SDL_INIT_VIDEO));
    
    int mode=1;

    SDL_Window* window= scp(SDL_CreateWindow("Maditor", 0,0, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE));
    SDL_Renderer* renderer= scp(SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED));

    Font font = font_load_from_file(renderer, "./charmap-oldschool_white.png");
    //creating event loop
    

    bool quit=false;
    while (!quit){
        SDL_Event event={0}; //initialising the event struct
        while (SDL_PollEvent(&event)){
            switch (event.type){
                case SDL_QUIT: {
                    quit =true;
                } break;

                case SDL_KEYDOWN: {     //try to understand how eventhough sdl keydown catches everything text still goes to textinput case
                    //printf("In event \n");
                    switch(event.key.keysym.sym){ 
                        case SDLK_BACKSPACE: {
                            //printf("In backspace \n");
                            if (buffer_size > 0){
                                buffer_size -= 1;
                                buffer_cursor=buffer_size;
                            }
                         } break;

                         case SDLK_F5: {
                            r=rand_r(&seed); //this will provide different value for multi color typing mode (2) 
                            mode = (mode +1) % 3;
                         }
                    }
                } break;

                case SDL_TEXTINPUT: {
                    //printf(" Typing \n");
                    size_t text_size = strlen(event.text.text);
                    const size_t free_space = BUFFER_CAPACITY - buffer_size;
                    if (text_size>free_space){
                        text_size=buffer_size;
                    }
                    memcpy(buffer + buffer_size, event.text.text, text_size);
                    buffer_size += text_size;
                    buffer_cursor=buffer_size;
                } break;
            }
        }
        scc(SDL_SetRenderDrawColor(renderer,0,0,0,0)); //sets the color to be used by the renderer for operations
        scc(SDL_RenderClear(renderer)); //clears the renderer with the set color

        render_text_sized(renderer, &font, buffer,buffer_size, vec2f(0.0, 0.0), FONT_COLOR, FONT_SCALE, r, mode);
        render_cursor(renderer, &font, FONT_COLOR);
        render_char_len(renderer, &font, FONT_COLOR, FONT_SCALE);
        SDL_RenderPresent(renderer); //updates the screen
    }

    SDL_Quit();
    return 0;
}