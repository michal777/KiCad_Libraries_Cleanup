#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ftw.h>

#define MAX_LIB_NR 500
#define MAX_NAME_LENGTH 500
#define BUFFER_SIZE 2000


FILE *file_log_all_wrl;						//list of all 3d .wrl files

int search(const char *nazwa, const struct stat *statystyka, int typ)
{
	if (strncmp(nazwa + strlen(nazwa) - strlen(".wrl"), ".wrl", strlen(".wrl")) == 0)
		fprintf(file_log_all_wrl, "%s\n", nazwa);
    return 0;
}



int
main (void)
{

//////////////////////////////////////////////////////////////////////
	DIR *dir_pretty_old;						// directory containing original .pretty
	DIR *dir_pretty_new;						// directory containing new .pretty	
	DIR *dir_library_old;						// directory containing original .kicad_mod
	FILE *file_kicad_mod_old;					//original .kicad_mod file
	FILE *file_kicad_mod_new;					//new .kicad_mod file (in new location, with changes)
	FILE *file_3d_old;							//original 3d files .wrl or .wings
	FILE *file_3d_new;							//new 3d files .wrl or .wings (new locations and names)
	FILE *file_log_copied_wrl;					//list of copied 3d .wrl files
	FILE *file_log_broken_wrl;					//list of copied 3d .wrl files
	FILE *file_log_unused_wrl;					//list of unused 3d .wrl files
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
	int i, j, k, l, m, n;						//counters
	char read_char;								//char variable
	int read_int;								//int variable
	char read_string_1[BUFFER_SIZE];			//string variable
	char read_string_2[BUFFER_SIZE];			//string variable
	int found;									//found flag - set to '1' if .wrl was used
	char buffer_in[BUFFER_SIZE];				//buffer for line read from file
	char buffer_out[BUFFER_SIZE];				//buffer for line to write to file
//////////////////////////////////////////////////////////////////////


	if((file_log_copied_wrl = fopen("./log/copied_wrl.log", "wt")) == NULL)
	{
		perror ("Couldn't open copied_wrl.log file");
		exit(1);
	}
	if((file_log_broken_wrl = fopen("./log/broken_wrl.log", "wt")) == NULL)
	{
		perror ("Couldn't open copied_wrl.log file");
		exit(1);
	}
	if((file_log_all_wrl = fopen("./log/all_wrl.log", "wt")) == NULL)
	{
		perror ("Couldn't open all_wrl.log file");
		exit(1);
	}
	if((file_log_unused_wrl = fopen("./log/unused_wrl.log", "wt")) == NULL)
	{
		perror ("Couldn't open unused_wrl.log file");
		exit(1);
	}
	

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
      	closedir (dir_pretty_old);
      	strcpy(pretty_list[i].d_name, "_");	//add "_" at the end of the table to recognize when it finishes
    }
	else
		perror ("Couldn't open the directory");

//////////////////////////////////////////////////////////////////////
//	do following operations on all .pretty libraries
//////////////////////////////////////////////////////////////////////
	for (i = 0; strcmp(pretty_list[i].d_name, "_") != 0; ++i)
	{		
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
		  	closedir (dir_library_old);
		  	strcpy(footprint_list[j].d_name, "_");
		}
		else
			perror ("Couldn't open the directory");

		strcpy(new_pretty_path, "./Pretty/");
		strcat(new_pretty_path, pretty_list[i].d_name);
		mkdir (new_pretty_path, 0777);	//create new .pretty directory for modified footprints
		dir_pretty_new = opendir(new_pretty_path);

		strcpy(new_3d_dir_path, "./packages3d/");
		strncat(new_3d_dir_path, pretty_list[i].d_name, strlen(pretty_list[i].d_name) - strlen(".pretty"));
		mkdir (new_3d_dir_path, 0777);	//create new directory for renamed 3d models
						
		dir_library_old = opendir (old_pretty_path);	//open old .pretty with footprints to modify		
		
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
						


						if ((strncmp("(module ", buffer_in + BUFFER_SIZE - l, strlen("(module ")) == 0))	//search "  model " in line, ommit if there was EOF
						{
							for(n = 0; (buffer_in[BUFFER_SIZE - l + strlen("(module ") + n]) != ' '; ++n)
								{}		//
							for(m = 0; (buffer_in[BUFFER_SIZE - l + strlen("(module ") + n + m]) != '\n'; ++m)		//save rest of line, following "(module "
								read_string_1[m] = buffer_in[BUFFER_SIZE - l + strlen("(module ") + n + m];
							read_string_1[m] = '\0';
						
							strcpy(buffer_out, "(module ");
							strncat(buffer_out, footprint_list[j].d_name, strlen(footprint_list[j].d_name) - strlen(".kicad_mod"));
							strcat(buffer_out, read_string_1);							
							for(m = 0; buffer_out[m] != '\0'; ++m)
								fputc(buffer_out[m], file_kicad_mod_new);	//write new line
							fputc('\n', file_kicad_mod_new);
						}
						else if ((strncmp("  (fp_text reference ", buffer_in + BUFFER_SIZE - l, strlen("  (fp_text reference ")) == 0))	//search "  (fp_text reference " in line, ommit if there was EOF
						{
							for(n = 0; (buffer_in[BUFFER_SIZE - l + strlen("  (fp_text reference ") + n]) != ' '; ++n)
								{}		//
							for(m = 0; (buffer_in[BUFFER_SIZE - l + strlen("  (fp_text reference ") + n + m]) != '\n'; ++m)		//save rest of line, following "  (fp_text reference "
								read_string_1[m] = buffer_in[BUFFER_SIZE - l + strlen("  (fp_text reference ") + n + m];
							read_string_1[m] = '\0';
						
							strcpy(buffer_out, "  (fp_text reference ");
							strncat(buffer_out, footprint_list[j].d_name, strlen(footprint_list[j].d_name) - strlen(".kicad_mod"));
							strcat(buffer_out, read_string_1);							
							for(m = 0; buffer_out[m] != '\0'; ++m)
								fputc(buffer_out[m], file_kicad_mod_new);	//write new line
							fputc('\n', file_kicad_mod_new);
						}
						else if ((strncmp("  (fp_text value ", buffer_in + BUFFER_SIZE - l, strlen("  (fp_text value ")) == 0))	//search "  (fp_text value " in line, ommit if there was EOF
						{
							for(n = 0; (buffer_in[BUFFER_SIZE - l + strlen("  (fp_text value ") + n]) != ' '; ++n)
								{}		//
							for(m = 0; (buffer_in[BUFFER_SIZE - l + strlen("  (fp_text value ") + n + m]) != '\n'; ++m)		//save rest of line, following "  (fp_text value "
								read_string_1[m] = buffer_in[BUFFER_SIZE - l + strlen("  (fp_text value ") + n + m];
							read_string_1[m] = '\0';
						
							strcpy(buffer_out, "  (fp_text value VAL**");
							strcat(buffer_out, read_string_1);							
							for(m = 0; buffer_out[m] != '\0'; ++m)
								fputc(buffer_out[m], file_kicad_mod_new);	//write new line
							fputc('\n', file_kicad_mod_new);
						}
						else if ((strncmp("    (effects (font ", buffer_in + BUFFER_SIZE - l, strlen("    (effects (font ")) == 0))	//search "    (effects (font " in line, ommit if there was EOF
						{
							strcpy(buffer_out, "    (effects (font (size 1 1) (thickness 0.15)))");
							for(m = 0; buffer_out[m] != '\0'; ++m)
								fputc(buffer_out[m], file_kicad_mod_new);	//write new line with new text size
							fputc('\n', file_kicad_mod_new);
						}
						else if ((strncmp("  (model ", buffer_in + BUFFER_SIZE - l, strlen("  (model ")) == 0))	//search "  model " in line, ommit if there was EOF
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
						else if (((strncmp("  (fp_line ", buffer_in + BUFFER_SIZE - l, strlen("  (fp_line ")) == 0) || (strncmp("  (fp_circle ", buffer_in + BUFFER_SIZE - l, strlen("  (fp_circle ")) == 0)))	//search "  (fp_line " in line, ommit if there was EOF
						{
							for(m = 0; ( strncmp( &buffer_in[BUFFER_SIZE - l + m], " (layer F.SilkS) (width ", strlen(" (layer F.SilkS) (width ") ) != 0 ) && m < l; ++m)		//save beginning of line
								read_string_1[m] = buffer_in[BUFFER_SIZE - l + m];
							read_string_1[m] = '\0';
							if( m != l )
							{
								strcpy( buffer_out, read_string_1 );
								strcat( buffer_out, " (layer F.SilkS) (width 0.15))" );
								for(m = 0; buffer_out[m] != '\0'; ++m)
									fputc(buffer_out[m], file_kicad_mod_new);	//write new line
								fputc('\n', file_kicad_mod_new);
							}
							else
								for(m = 0; m < l; ++m)
									fputc(buffer_in[BUFFER_SIZE - l + m], file_kicad_mod_new);	//if not found " (layer F.SilkS) (width 0.15))", write line to new file with no changes
						}					
						
						else
							for(m = 0; m < l; ++m)
								fputc(buffer_in[BUFFER_SIZE - l + m], file_kicad_mod_new);	//if not found eg. "  model ", write line to new file with no changes
						
					
					
					} while (read_char != EOF);
					//------------------------------------------------------
						
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
				{
					while((read_char = fgetc(file_3d_old)) != EOF)
						fputc(read_char, file_3d_new);	
					fprintf(file_log_copied_wrl, "%s\n", old_3d_path);					
					fclose(file_3d_new);
				}
				else
					perror ("Couldn't open the new .wrl file");
				fclose(file_3d_old);	
			}
			else
			{
				perror ("Couldn't open the old .wrl file");
				fprintf(file_log_broken_wrl, "%s\n", old_3d_path);					
			}
				
			
			
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
				{
					while((read_int = fgetc(file_3d_old)) != EOF)
						fputc(read_int, file_3d_new);
					fclose(file_3d_new);	
				}	
				else
					perror ("Couldn't open the new .wings file");
				fclose(file_3d_old);
			}
			else
				perror ("Couldn't open the old .wings file");
				
				
			// END copy and rename 3d models
			//-----------------------------			
		}
			
		closedir(dir_pretty_new);
		closedir(dir_library_old);
	}
	


    ftw("./packages3d_bck", search, 1);	//list all 3d .wrl files




	fclose(file_log_copied_wrl);		//close lists of used and all .wrl files, to open them for read
	fclose(file_log_all_wrl);
	if((file_log_copied_wrl = fopen("./log/copied_wrl.log", "rt")) == NULL)
	{
		perror ("Couldn't open copied_wrl.log file");
		exit(1);
	}
	if((file_log_all_wrl = fopen("./log/all_wrl.log", "rt")) == NULL)
	{
		perror ("Couldn't open all_wrl.log file");
		exit(1);
	}
	while(fgets(read_string_1, BUFFER_SIZE, file_log_all_wrl) != NULL)		//search unused .wrl files
	{
		found = 0;
  		fseek(file_log_copied_wrl, 0, 0);
		while(fgets(read_string_2, BUFFER_SIZE, file_log_copied_wrl) != NULL)
			if(strcmp(read_string_1, read_string_2) == 0)	//check if file name from list of all files exists on list of copied files
				found = 1;
		if(found == 0)
			fputs(read_string_1, file_log_unused_wrl);	//write to list of unused if not found in list of used
	}



	// copy unused 3d models
	//-----------------------------
	
	fclose(file_log_unused_wrl);
	if((file_log_unused_wrl = fopen("./log/unused_wrl.log", "rt")) == NULL)
	{
		perror ("Couldn't open copied_wrl.log file");
		exit(1);
	}
	j = 0;
	while(fgets(read_string_1, BUFFER_SIZE, file_log_unused_wrl) != NULL)	
	{	
		strcpy( old_3d_path, read_string_1 );
		old_3d_path[strlen(read_string_1) - 1] = '\0';
		strcpy( new_3d_path, "./unused_3d" );
		for(i = strlen(read_string_1); read_string_1[i] != '/'; --i)
			{}
		strcat( new_3d_path, read_string_1 + i);
		new_3d_path[strlen("./unused_3d") + strlen(read_string_1) - i - 1] = '\0';
		if((file_3d_old = fopen(old_3d_path, "rt")) != NULL)
		{
			if((file_3d_new = fopen(new_3d_path, "rt")) != NULL)
			{
				sprintf(read_string_2, "%d", ++j);
				strcat( new_3d_path, read_string_2 );
				fclose(file_3d_new);
			}				
			if((file_3d_new = fopen(new_3d_path, "wt")) != NULL)
			{
				while((read_char = fgetc(file_3d_old)) != EOF)
					fputc(read_char, file_3d_new);
				fclose(file_3d_new);
			}
			else
				perror ("Couldn't open the new .wrl file");
			fclose(file_3d_old);
		}
		else
			perror ("Couldn't open the old .wrl file");
			

		strcpy( old_3d_path, read_string_1 );
		old_3d_path[strlen(read_string_1) - strlen(".wrl") - 1] = '\0';
		strcat( old_3d_path, ".wings" );
		strcpy( new_3d_path, "./unused_3d" );
		for(i = strlen(read_string_1); read_string_1[i] != '/'; --i)
			{}
		strcat( new_3d_path, read_string_1 + i);
		new_3d_path[strlen("./unused_3d") - strlen(".wrl") + strlen(read_string_1) - i - 1] = '\0';
		strcat( new_3d_path, ".wings" );
		if((file_3d_old = fopen(old_3d_path, "rt")) != NULL)
		{
			if((file_3d_new = fopen(new_3d_path, "rt")) != NULL)
			{
				sprintf(read_string_2, "%d", ++j);
				strcat( new_3d_path, read_string_2 );
				fclose(file_3d_new);
			}
			if((file_3d_new = fopen(new_3d_path, "wt")) != NULL)
			{
				while((read_int = fgetc(file_3d_old)) != EOF)
					fputc(read_int, file_3d_new);
				fclose(file_3d_new);	
			}
			else
				printf ("Couldn't open the new .wings file, %s", read_string_1);
			fclose(file_3d_old);
		}
		else
			perror ("Couldn't open the old .wings file");

	}						
	// END copy unused 3d models
	//-----------------------------		



	fclose(file_log_copied_wrl);
	fclose(file_log_all_wrl);
	fclose(file_log_unused_wrl);	

	return 0;
}

