#include "config.h"

#include <stdio.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_log.h>
#include <stddef.h>

const char const config_path[] = "config.cfg";



edv_config cfg_defaults = {
#define CFG(x,arg,...) __VA_ARGS__,
	CFG_LIST
#undef CFG
};



typedef struct {
	cfg_type type;
	char* name;
	size_t offset; //offset into config struct
} cfg_name;

const cfg_name cfg_names[] = {

#define CFG(x,arg,y) {x##_cfg, #arg, offsetof(edv_config,arg)},
	CFG_LIST
#undef CFG
};

edv_color parse_color(char* hexstr) {

	char hexbuf[3];
	hexbuf[2] = '\0';

	if (SDL_strlen(hexstr) != 8) {
		return (edv_color) { 0 };
	}

	hexbuf[0] = hexstr[0];
	hexbuf[1] = hexstr[1];
	int r = SDL_strtol(hexbuf, NULL, 16);
	if (r < 0 || r > 255) {
		return (edv_color) { 0 };
	}
	hexbuf[0] = hexstr[2];
	hexbuf[1] = hexstr[3];
	int g = SDL_strtol(hexbuf, NULL, 16);
	if (g < 0 || g > 255) {
		return (edv_color) { 0 };
	}
	hexbuf[0] = hexstr[4];
	hexbuf[1] = hexstr[5];
	int b = SDL_strtol(hexbuf, NULL, 16);
	if (b < 0 || b > 255) {
		return (edv_color) { 0 };
	}
	hexbuf[0] = hexstr[6];
	hexbuf[1] = hexstr[7];
	int a = SDL_strtol(hexbuf, NULL, 16);
	if (a < 0 || a > 255) {
		return (edv_color) {0};
	}
	edv_color c = (edv_color){ r,g,b,a };

	return c;
}

void unload_config(edv_config *cfg) {
	SDL_free(cfg->default_font);

	SDL_free(cfg);
}

void print_arg(FILE *f, cfg_type type, const char* name, void* value) {

	char buf[256];

	switch (type)
	{
	case edv_string_cfg:
		SDL_snprintf(buf, 256, "%s", *((char**)value));
		break;
	case edv_color_cfg:
		SDL_snprintf(buf, 256, "%.2X%.2X%.2X%.2X", ((edv_color*)value)->r & 0xff, ((edv_color*)value)->g & 0xff, ((edv_color*)value)->b & 0xff, ((edv_color*)value)->a & 0xff);
		break;
	case edv_int_cfg:
		SDL_snprintf(buf, 256, "%d", *((int*)value));
		break;
	default:
		break;
	}

	fprintf(f, "%s=%s\n", name, buf);
}

edv_config *load_config(void) {

	edv_config* cfg = SDL_malloc(2*sizeof(edv_config));


	if (cfg == NULL) return NULL;

	memcpy(cfg, &cfg_defaults, sizeof(edv_config));

	cfg->default_font = SDL_malloc(1024);
	memcpy(cfg->default_font, cfg_defaults.default_font, strlen(cfg_defaults.default_font)+1);



	if (SDL_GetPathInfo(config_path, NULL)) {



		char buf[1024];//buffer massive so that we shouldnt ever run out of memory for this



		FILE* f = fopen(config_path, "r");


		while (fgets(buf, 1024, f)) {

			char* save = 0;
			char* save1 = 0;


			char* name = SDL_strtok_r(buf, "=", &save);
			if (name == NULL) continue;

			char* rvalue = SDL_strtok_r(NULL, "\n", &save);

			while (*rvalue == ' ') rvalue++; //skip over spaces

			rvalue = SDL_strtok_r(rvalue, " ", save1); // cut out end spaces

			if (rvalue == NULL) {
				continue;
			}
			size_t name_size = (sizeof(cfg_names)) / sizeof(cfg_names[0]);

			edv_string str;
			edv_color col;
			
			for (size_t i = 0; i < name_size; i++)
			{
				if (strcmp(name, cfg_names[i].name) == 0) { //string matches
					edv_config* c = (edv_config*)(((char*)cfg) + cfg_names[i].offset);

					switch (cfg_names[i].type)
					{
					case edv_color_cfg:
						col = parse_color(rvalue);
						//*(edv_color*)(cfg + cfg_names[i].offset) = col;
						memcpy(c, &col, sizeof(edv_color));

						goto next;
					case edv_string_cfg:
						str = *(edv_string*)c;
						SDL_strlcpy(str, rvalue, SDL_strlen(rvalue)+1);
						goto next;
					case edv_int_cfg:
						*(int*)c = SDL_atoi(rvalue);
						goto next;
					default:
						break;
					}
				}
			}
		next:
			;
		}

		fclose(f);

		//cfg = SDL_malloc(sizeof(edv_config));
		//SDL_memcpy(cfg, &cfgs, sizeof(edv_config)); //i have to do this bullshit to stop the address sanitiser from panicking for some reason
		return cfg;
	}
	else
	{

		
		SDL_Log("No Config file found. Generating...\n");
		FILE* f = fopen(config_path, "w");
		if (f == NULL) {
			SDL_Log("Failed to generate config file\n");
			
			return cfg;
		}


		//evil define to make it more '''dynamic'''
		//basically, it creates a temporary variable with the default values stored in the var args of the define, 
		//then passes a pointer to that into a function that prints the args, as well as a string representation of the name and the type (gotten by appended _cfg to the edv_type)
#define CFG(x,arg,...) x arg##_tmp = __VA_ARGS__;  print_arg(f,x##_cfg,#arg,&arg##_tmp);
		CFG_LIST
#undef CFG

		fclose(f);

		return cfg;
	}
}

