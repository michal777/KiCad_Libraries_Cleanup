#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

#define MAX_LIB_NR 500
#define MAX_NAME_LENGTH 500
#define BUFFER_SIZE 2000

int
main (void)
{

//////////////////////////////////////////////////////////////////////
	DIR *dir_pretty_old;						// directory containing original .pretty
	DIR *dir_library_old;						// directory containing original .kicad_mod
	FILE *file_kicad_mod_old;					//original .kicad_mod file
	FILE *file_kicad_mod_new;					//new .kicad_mod file (in new location, with changes)
	FILE *file_3d_old;							//original 3d files .wrl or .wings
	FILE *file_3d_new;							//new 3d files .wrl or .wings (new locations and names)
	struct dirent *ep;							//structure containing name of file get by readdir
	struct dirent pretty_list[MAX_LIB_NR];		//array containing list of .pretty libraries
	struct dirent footprint_list[MAX_LIB_NR];	//array containing list of .kicad_mod footprints
	char new_pretty_path[MAX_NAME_LENGTH];		//new path to .pretty library
	char old_pretty_path[MAX_NAME_LENGTH];		//original path to .pretty library
	char old_kicad_mod_path[MAX_NAME_LENGTH];	//original path to .kicad_moc footprint
	char new_kicad_mod_path[MAX_NAME_LENGTH];	//new path to .kicad_moc footprint
	char path_3d_old[MAX_NAME_LENGTH];			//path to 3d module read from .kicad_mod file
	char old_3d_path[MAX_NAME_LENGTH];			//original path to 3d file (.wrl or .wings)
	char new_3d_path[MAX_NAME_LENGTH];			//new path to renamed 3d file (.wrl or .wings)
	char new_3d_dir_path[MAX_NAME_LENGTH];		//new path to directory corresponding with proper .pretty
	int i, j, k, l, m;							//counters
	char read_char;								//char variable
	char buffer_in[BUFFER_SIZE];				//buffer for line read from file
	char buffer_out[BUFFER_SIZE];				//buffer for line to write to file
//////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////
//	get list of .pretty libraries to array
//////////////////////////////////////////////////////////////////////
	dir_pretty_old = opendir ("./Pretty_bck");
	if (dir_pretty_old != NULL)
	{
		for (i = 0; ep = readdir (dir_pretty_old); ++i)
			if (strncmp(ep->d_name + strlen(ep->d_name) - strlen(".pretty"), ".pretty", strlen(".pretty")) == 0)	//add only .pretty files
				pretty_list[i] = *ep;
			else
				--i;
      	(void) closedir (dir_pretty_old);
      	strcpy(pretty_list[i].d_name, "_");	//add "_" at the end of the table to recognize when it finishes
    }
	else
		perror ("Couldn't open the directory");

//////////////////////////////////////////////////////////////////////
//	do following operations on all .pretty libraries
//////////////////////////////////////////////////////////////////////
	for (i = 0; strcmp(pretty_list[i].d_name, "_") != 0; ++i)
	{

		strcpy(new_pretty_path, "./Pretty/");
		strcat(new_pretty_path, pretty_list[i].d_name);
		mkdir (new_pretty_path, 0777);	//create new .pretty directory for modified footprints

		strcpy(new_3d_dir_path, "./packages3d/");
		strncat(new_3d_dir_path, pretty_list[i].d_name, strlen(pretty_list[i].d_name) - strlen(".pretty"));
		mkdir (new_3d_dir_path, 0777);	//create new directory for renamed 3d models
		
		//---------------------------------------------------------------
		//	get list of .kicad_mod footprints in current .pretty library
		//---------------------------------------------------------------
		strcpy(old_pretty_path, "./Pretty_bck/");
		strcat(old_pretty_path, pretty_list[i].d_name);		
		dir_library_old = opendir (old_pretty_path);	//open old .pretty with footprints to modify
		if (dir_library_old != NULL)
		{
			for (j = 0; ep = readdir (dir_library_old); ++j)
				if (strncmp(ep->d_name + strlen(ep->d_name) - strlen(".kicad_mod"), ".kicad_mod", strlen(".kicad_mod")) == 0)
					footprint_list[j] = *ep;
				else
					--j;
		  	(void) closedir (dir_library_old);
		  	strcpy(footprint_list[j].d_name, "_");
		}
		else
			perror ("Couldn't open the directory");
		
		
		//HERE OPERATIONS ON FOOTPRINTS AND 3D MODELS ARE DONE
		//---------------------------------------------------------------
		//	do following operations on all footprints in current library
		//---------------------------------------------------------------		
		for (j = 0; strcmp(footprint_list[j].d_name, "_") != 0; ++j)
		{
			strcpy(old_kicad_mod_path, "./Pretty_bck/");		//prepare pathes to read original and write to new .kicad_mod file
			strcat(old_kicad_mod_path, pretty_list[i].d_name);		
			strcat(old_kicad_mod_path, "/");
			strcat(old_kicad_mod_path, footprint_list[j].d_name);				
			strcpy(new_kicad_mod_path, "./Pretty/");
			strcat(new_kicad_mod_path, pretty_list[i].d_name);		
			strcat(new_kicad_mod_path, "/");
			strcat(new_kicad_mod_path, footprint_list[j].d_name);
							
			if ((file_kicad_mod_old = fopen(old_kicad_mod_path, "rt")) != NULL)
			{
				if ((file_kicad_mod_new = fopen(new_kicad_mod_path, "wt")) != NULL)
				{
				
					//HERE .kicad_mod FOOTPRINTS ARE MODIFIED
					strcpy(path_3d_old, "0");
					do
					{	
						l = 0;		//reset counter of characters in line
						do
						{
							if ((read_char = fgetc(file_kicad_mod_old)) != EOF)	//get character from file
							{
								for (k = 0; k < BUFFER_SIZE - 1; ++k)
									buffer_in[k] = buffer_in[k + 1];	//load line to shift register
								buffer_in[BUFFER_SIZE - 1] = read_char;
								++l;									//increment counter of characters read from line
							}
						} while ((read_char != '\n') && (read_char != EOF));	//check if line is read
						

						if ((strncmp("  (model ", buffer_in + BUFFER_SIZE - l, strlen("  (model ")) == 0) && (read_char != EOF))	//search "  model " in line, ommit if there was EOF
						{
							for(m = 0; (buffer_in[BUFFER_SIZE - l + strlen("  (model ") + m]) != '\n'; ++m)		//save path to 3d to path_3d_old (path should finish with '\n'
								path_3d_old[m] = buffer_in[BUFFER_SIZE - l + strlen("  (model ") + m];
							path_3d_old[m] = '\0';

							strcpy(buffer_out, "  (model ");
							strncat(buffer_out, pretty_list[i].d_name, strlen(pretty_list[i].d_name) - strlen(".pretty"));
							strcat(buffer_out, "/");
							strncat(buffer_out, footprint_list[j].d_name, strlen(footprint_list[j].d_name) - strlen(".kicad_mod"));	
							strcat(buffer_out, ".wrl");
							for(m = 0; buffer_out[m] != '\0'; ++m)
								fputc(buffer_out[m], file_kicad_mod_new);	//write new line with path to new 3d model
							fputc('\n', file_kicad_mod_new);
						}
						else if (read_char != EOF)
							for(m = 0; m < l; ++m)
								fputc(buffer_in[BUFFER_SIZE - l + m], file_kicad_mod_new);	//if not found "  model ", write line to new file with no changes
						
					} while (read_char != EOF);
				
						
				}
				else
					perror ("Couldn't open the new .kicad_mod file");
				fclose(file_kicad_mod_new);	
			}
			else
				perror ("Couldn't open the old .kicad_mod file");
			fclose(file_kicad_mod_old);
			
			// copy and rename 3d models
			//-----------------------------
			
			strcpy(old_3d_path, "./packages3d_bck/");
			strcat(old_3d_path, path_3d_old);			
			strcpy(new_3d_path, "./packages3d/");
			strncat(new_3d_path, pretty_list[i].d_name, strlen(pretty_list[i].d_name) - strlen(".pretty"));
			strcat(new_3d_path, "/");
			strncat(new_3d_path, footprint_list[j].d_name, strlen(footprint_list[j].d_name) - strlen(".kicad_mod"));				
			strcat(new_3d_path, ".wrl");

			if((file_3d_old = fopen(old_3d_path, "rt")) != NULL)
			{
				if((file_3d_new = fopen(new_3d_path, "wt")) != NULL)
					while((read_char = fgetc(file_3d_old)) != EOF)
						fputc(read_char, file_3d_new);	
				else
					perror ("Couldn't open the new .wrl file");
				if(file_3d_new != NULL)
					fclose(file_3d_new);	
			}
			else
				perror ("Couldn't open the old .wrl file");
			if(file_3d_old != NULL)
				fclose(file_3d_old);
			
			
			strcpy(old_3d_path, "./packages3d_bck/");
			strncat(old_3d_path, path_3d_old, strlen(path_3d_old) - strlen(".wrl"));
			strcat(old_3d_path, ".wings");
			strcpy(new_3d_path, "./packages3d/");
			strncat(new_3d_path, pretty_list[i].d_name, strlen(pretty_list[i].d_name) - strlen(".pretty"));
			strcat(new_3d_path, "/");
			strncat(new_3d_path, footprint_list[j].d_name, strlen(footprint_list[j].d_name) - strlen(".kicad_mod"));				
			strcat(new_3d_path, ".wings");

			if((file_3d_old = fopen(old_3d_path, "rt")) != NULL)
			{
				if((file_3d_new = fopen(new_3d_path, "wt")) != NULL)
					while((read_char = fgetc(file_3d_old)) != EOF)
						fputc(read_char, file_3d_new);	
				else
					perror ("Couldn't open the new .wings file");
				if(file_3d_new != NULL)
					fclose(file_3d_new);	
			}
			else
				perror ("Couldn't open the old .wings file");
			if(file_3d_old != NULL)
				fclose(file_3d_old);
				
			// END copy and rename 3d models
			//-----------------------------			
		}
		
	}



	return 0;
}

