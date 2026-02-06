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

int parse_color(char* hexstr,edv_color *col) {

	char hexbuf[9];
	hexbuf[8] = '\0';

	if (SDL_strlen(hexstr) != 8) {
		return 0;
	}
	unsigned long hex = SDL_strtoul(hexstr, NULL, 16);
	*col = (edv_color){ ((hex >> 24) & 0xFF),((hex >> 16) & 0xFF),((hex >> 8) & 0xFF), ((hex) & 0xFF) };
	return 1;
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

	fprintf(f, "%s = %s\n", name, buf);
}

char* trim(char* str) {
	//remove leading and trailing spaces
	while (*str == ' ') str++; //skip over spaces

	char* d = str;
	while (*d != ' ' && *d != '\0') {
		d++;
	}
	*d = '\0';

	return str;
}

edv_config *load_config(void) {

	edv_config* cfg = SDL_malloc(2*sizeof(edv_config));


	if (cfg == NULL) return NULL;

	memcpy(cfg, &cfg_defaults, sizeof(edv_config));

	cfg->default_font = SDL_malloc(1024);
	memcpy(cfg->default_font, cfg_defaults.default_font, strlen(cfg_defaults.default_font)+1);



	if (SDL_GetPathInfo(config_path, NULL)) {



		char buf[1024];//buffer massive so that we shouldnt ever run out of memory for this

		size_t name_size = (sizeof(cfg_names)) / sizeof(cfg_names[0]);

		//values to use in the for loop
		edv_string str;
		edv_color col;


		FILE* f = fopen(config_path, "r");


		while (fgets(buf, 1024, f)) {

			char* save = 0;
			char* save1 = 0;


			char* name = SDL_strtok_r(buf, "=", &save);
			if (name == NULL) continue;

			name = trim(name);

			char* rvalue = SDL_strtok_r(NULL, "\n", &save); // gets the value of the config and has the added bonus of stripping off any newlines

			if (rvalue == NULL) continue; // only happens when its 'name=' and the rvalue is completely blank

			//remove leading and trailing spaces
			while (*rvalue == ' ') rvalue++; //skip over spaces

			rvalue = trim(rvalue);

			if (rvalue == NULL) {
				continue;
			}

			for (size_t i = 0; i < name_size; i++)
			{
				if (strcmp(name, cfg_names[i].name) == 0) { //string matches
					edv_config* c = (edv_config*)(((char*)cfg) + cfg_names[i].offset);

					switch (cfg_names[i].type)
					{
					case edv_color_cfg:

						if (!parse_color(rvalue, &col)) {
							goto invalid_config;
						}
						memcpy(c, &col, sizeof(edv_color));

						goto next; // i dont care that goto is 'bad practice', im not doing some bullshit to break out of a nested loop when i can just jump directly out
					case edv_string_cfg:
						SDL_strlcpy(*(edv_string*)c, rvalue, SDL_strlen(rvalue)+1);
						goto next;
					case edv_int_cfg:
						*(int*)c = SDL_atoi(rvalue);
						goto next;
					default:
						printf("Unknown config type for config '%s'\n", name);
						break;
					}
				}
			}
			
		invalid_config:
			printf("Invalid config: %s = %s\n", name, rvalue);
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

