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


FILE *G_file_log_all_wrl;			//list of all 3d .wrl files
FILE *G_file_log_all_wings;			//list of all 3d .wings files
FILE *G_file_log_all_not_3d;		//list of all not .wrl nor .wings files in ./packages3d_bck

/////////////////////// funtions ///////////////////////////////
int search_wrl (const char *name, const struct stat *statystyka, int type)		//function used by ftw (from ftw.h) to find all .wrl in directory tree
{
	if (strncmp (name + strlen (name) - strlen (".wrl"), ".wrl", strlen (".wrl")) == 0)		//write path to file if it ends with ".wrl"
		fprintf (G_file_log_all_wrl, "%s\n", name);
    return 0;
}
int search_wings (const char *name, const struct stat *statystyka, int type)		//function used by ftw (from ftw.h) to find all .wings in directory tree
{
	if (strncmp (name + strlen (name) - strlen (".wings"), ".wings", strlen (".wings")) == 0)		//write path to file if it ends with ".wings"
		fprintf (G_file_log_all_wings, "%s\n", name);
    return 0;
}
int search_not_3d (const char *name, const struct stat *statystyka, int type)		//function used by ftw (from ftw.h) to find all not .wings nor .wrl in directory tree
{
	if ((strncmp (name + strlen (name) - strlen (".wrl"), ".wrl", strlen (".wrl")) != 0) && (strncmp (name + strlen (name) - strlen (".wings"), ".wings", strlen (".wings")) != 0) && (type == FTW_F))		//write path to file if it doesn't end with ".wings" nor ".wrl" and is file
		fprintf (G_file_log_all_not_3d, "%s\n", name);
    return 0;
}


int main (void)
{

	DIR *dir_pretty_old;						// directory containing original .pretty
	DIR *dir_pretty_new;						// directory containing new .pretty	
	DIR *dir_library_old;						// directory containing original .kicad_mod

	FILE *file_kicad_mod_old;					//original .kicad_mod file
	FILE *file_kicad_mod_new;					//new .kicad_mod file (in new location, with changes)
	FILE *file_3d_old;							//original 3d files .wrl or .wings
	FILE *file_3d_new;							//new 3d files .wrl or .wings (new locations and names)

	FILE *file_log_copied_wrl;					//list of copied 3d .wrl files
	FILE *file_log_broken_link;					//list of copied 3d .wrl files
	FILE *file_log_unused_wrl;					//list of unused 3d .wrl files
	FILE *file_log_not_exported_wings;			//list of not exported 3d .wings files
	FILE *file_log_wrl_no_source;				//list of .wrl files without .wings files
	
	struct dirent *ep;							//structure containing name of file got by readdir
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
	int flag;									//flag to find eg unused models
	int model_link_exists;						//flag to check if .kicad_mod has link to 3d
	char string_buffer_1[BUFFER_SIZE];			//string variable
	char string_buffer_2[BUFFER_SIZE];			//string variable
	char buffer_in[BUFFER_SIZE];				//buffer for line read from file
	char buffer_out[BUFFER_SIZE];				//buffer for line to write to file





//////////////////////////////////////////////////////////////////////
//	get list of .pretty libraries to array
//////////////////////////////////////////////////////////////////////
	dir_pretty_old = opendir ("./Pretty_bck");
	if (dir_pretty_old != NULL)
	{
		for (i = 0; (ep = readdir (dir_pretty_old)) != NULL; ++i)
		{
			if (strncmp (ep->d_name + strlen (ep->d_name) - strlen (".pretty"), ".pretty", strlen (".pretty")) == 0)		//add only .pretty files
				pretty_list[i] = *ep;
			else
				--i;
		}
      	closedir (dir_pretty_old);
      	strcpy (pretty_list[i].d_name, "||\\//||");	//add "||\\//||" at the end of the table to recognize when it finishes
    }
	else
	{
		perror ("Couldn't open ./Pretty_bck directory");
		exit (1);
	}
	
//////////////// open log files to write ////////////////
	mkdir ("log", 0777);		//create directories for new files
	mkdir ("packages3d", 0777);
	mkdir ("Pretty", 0777);
	mkdir ("unused_3d", 0777);
	
//////////////// open log files to write ////////////////
	if ((file_log_copied_wrl = fopen ("./log/copied_wrl.log", "wt")) == NULL)
	{
		perror ("Couldn't open copied_wrl.log file");
		exit (1);
	}
	if ((file_log_broken_link = fopen ("./log/broken_link.log", "wt")) == NULL)
	{
		perror ("Couldn't open broken_link.log file");
		exit (1);
	}
	if ((file_log_wrl_no_source = fopen ("./log/wrl_no_source.log", "wt")) == NULL)
	{
		perror ("Couldn't open wrl_no_source.log file");
		exit (1);
	}
	
//////////////////////////////////////////////////////////////////////
//	do following operations on all .pretty libraries
//////////////////////////////////////////////////////////////////////
	for (i = 0; strcmp (pretty_list[i].d_name, "||\\//||") != 0; ++i)
	{
		//---------------------------------------------------------------
		//	get list of .kicad_mod footprints in current .pretty library
		//---------------------------------------------------------------	
		sprintf(old_pretty_path,"./Pretty_bck/%s", pretty_list[i].d_name);
		dir_library_old = opendir (old_pretty_path);		//open old .pretty with footprints to modify
		if (dir_library_old != NULL)
		{
			for (j = 0; (ep = readdir (dir_library_old)) != NULL; ++j)
			{
				if (strncmp (ep->d_name + strlen (ep->d_name) - strlen (".kicad_mod"), ".kicad_mod", strlen (".kicad_mod")) == 0)		//add only .kicad_mod files
					footprint_list[j] = *ep;
				else
					--j;
			}
		  	closedir (dir_library_old);
		  	strcpy (footprint_list[j].d_name, "||\\//||");		//add "||\\//||" at the end of the table to recognize when it finishes
		}
		else
		{
			perror ("Couldn't open  ./Pretty_bck/[FOOTPRINTNAME] directory");
			exit (1);
		}

		sprintf(new_pretty_path,"./Pretty/%s", pretty_list[i].d_name);
		mkdir (new_pretty_path, 0777);		//create new .pretty directory for modified footprints
		sprintf(new_3d_dir_path,"./packages3d/%.*s", (unsigned int)(strlen(pretty_list[i].d_name) - strlen(".pretty")), pretty_list[i].d_name);
		mkdir (new_3d_dir_path, 0777);		//create new directory for renamed 3d models
						
		
		//HERE OPERATIONS ON FOOTPRINTS AND 3D MODELS ARE DONE
		//---------------------------------------------------------------
		//	do following operations on all footprints in current library
		//---------------------------------------------------------------		
		for (j = 0; strcmp(footprint_list[j].d_name, "||\\//||") != 0; ++j)
		{
			model_link_exists = 0;		//reset flag to check if there is link to model
			sprintf(old_kicad_mod_path,"./Pretty_bck/%s/%s", pretty_list[i].d_name, footprint_list[j].d_name);		//prepare path to read original .kicad_mod file
			sprintf(new_kicad_mod_path,"./Pretty/%s/%s", pretty_list[i].d_name, footprint_list[j].d_name);		//prepare path to new .kicad_mod file
			
			if ((file_kicad_mod_old = fopen(old_kicad_mod_path, "rt")) != NULL)
			{
				if ((file_kicad_mod_new = fopen(new_kicad_mod_path, "wt")) != NULL)
				{		
					//HERE .kicad_mod FOOTPRINTS ARE MODIFIED
					for (fgets (buffer_in, BUFFER_SIZE, file_kicad_mod_old); !feof (file_kicad_mod_old); fgets (buffer_in, BUFFER_SIZE, file_kicad_mod_old))		//read lines of .kicad_mod file until reaches EOF (must be \n before EOF)
					{	
						
						if ((strncmp("(module ", buffer_in, strlen("(module ")) == 0))		//if line starts with "(module  " it will be modified
						{
							if (buffer_in[strlen("(module ")] == '"')		//it's possible that this string contains whitespaces, then it in must be between double quotes
							{
								for(n = 0; buffer_in[strlen("(module ") + n + 1] != '"'; ++n)		//must be ...+n+1 to ommit first "
									{;}
								n = n + 2;		//here, after adding 2, strlen("(module ") + n points at whitespace after module name in quotation
							}
							else		//if there are no doble quotes check how many characters has the string, count until whitespace
								for(n = 0; (buffer_in[strlen("(module ") + n]) != ' '; ++n)
									{;}		//if there are no quotation, when for loop ends, strlen("(module ") + n points at whitespace after module name		
							for(m = 0; (buffer_in[strlen("(module ") + n + m - 1]) != '\0'; ++m)		//save rest of line, following "(module " and MODULENAME, copy also \0
								string_buffer_1[m] = buffer_in[strlen("(module ") + n + m];
							sprintf(buffer_out,"(module %.*s%s", (unsigned int)(strlen(footprint_list[j].d_name) - strlen(".kicad_mod")), footprint_list[j].d_name, string_buffer_1);
							fputs (buffer_out, file_kicad_mod_new);		//print line modified above
						}
						else if ((strncmp("  (fp_text reference ", buffer_in, strlen("  (fp_text reference ")) == 0))		// modify "  (fp_text reference " similar as "module "
						{
							if (buffer_in[strlen("  (fp_text reference ")] == '"')
							{
								for(n = 0; buffer_in[strlen("  (fp_text reference ") + n + 1] != '"'; ++n)
									{;}
								n = n + 2;
							}
							else
								for(n = 0; (buffer_in[strlen("  (fp_text reference ") + n]) != ' '; ++n)
									{;}			
							for(m = 0; (buffer_in[strlen("  (fp_text reference ") + n + m - 1]) != '\0'; ++m)
								string_buffer_1[m] = buffer_in[strlen("  (fp_text reference ") + n + m];				
							strcpy(buffer_out, "  (fp_text reference ");
							sprintf(buffer_out,"  (fp_text reference %.*s%s", (unsigned int)(strlen(footprint_list[j].d_name) - strlen(".kicad_mod")), footprint_list[j].d_name, string_buffer_1);		
							fputs (buffer_out, file_kicad_mod_new);
						}
						else if ((strncmp("  (fp_text value ", buffer_in, strlen("  (fp_text value ")) == 0))
						{
							if (buffer_in[strlen("  (fp_text value ")] == '"')
							{
								for(n = 0; buffer_in[strlen("  (fp_text value ") + n + 1] != '"'; ++n)
									{;}
								n = n + 2;
							}
							else
								for(n = 0; (buffer_in[strlen("  (fp_text value ") + n]) != ' '; ++n)
									{;}			
							for(m = 0; (buffer_in[strlen("  (fp_text value ") + n + m - 1]) != '\0'; ++m)
								string_buffer_1[m] = buffer_in[strlen("  (fp_text value ") + n + m];						
							sprintf(buffer_out,"  (fp_text value VAL**%s", string_buffer_1);									
							fputs (buffer_out, file_kicad_mod_new);
						}
						else if (strncmp("    (effects (font ", buffer_in, strlen("    (effects (font ")) == 0)
						{
							fputs ("    (effects (font (size 1 1) (thickness 0.15)))\n", file_kicad_mod_new);
						}
						else if ((strncmp("  (fp_line ", buffer_in, strlen("  (fp_line ")) == 0) || (strncmp("  (fp_circle ", buffer_in, strlen("  (fp_circle ")) == 0))
						{
							for (m = 0; (strncmp(buffer_in + m, " (layer F.SilkS) (width ", strlen(" (layer F.SilkS) (width ")) != 0) && m < strlen(buffer_in); ++m)
								string_buffer_1[m] = buffer_in[m];
							string_buffer_1[m] = '\0';
							if (m < strlen(buffer_in))
								fputs (strcat (string_buffer_1, " (layer F.SilkS) (width 0.15))\n"), file_kicad_mod_new);
							else
								fputs (buffer_in, file_kicad_mod_new);	//if not found " (layer F.SilkS) (width 0.15))", write line to new file with no changes
						}		
						else if (strncmp("  (model ", buffer_in, strlen("  (model ")) == 0)		//search line with link to 3d model
						{
							if (buffer_in[strlen("  (model ")] == '"')
								for(m = 0; buffer_in[strlen("  (model ") + m + 1] != '"'; ++m)		//save path to 3d to path_3d_old (path should finish with '"')
									path_3d_old[m] = buffer_in[strlen("  (model ") + m + 1];		//path begins after "  (model "", strlen("  (model ") points at "
							else
								for(m = 0; (buffer_in[strlen("  (model ") + m]) != '\n'; ++m)		//save path to 3d to path_3d_old (path should finish with '\n'
									path_3d_old[m] = buffer_in[strlen("  (model ") + m];
							path_3d_old[m] = '\0';		// \0 instead of \n or "
							sprintf(buffer_out,"  (model %.*s/%.*s.wrl\n", (unsigned int)(strlen(pretty_list[i].d_name) - strlen(".pretty")), pretty_list[i].d_name, (unsigned int)(strlen(footprint_list[j].d_name) - strlen(".kicad_mod")), footprint_list[j].d_name);
							fputs (buffer_out, file_kicad_mod_new);
							model_link_exists = 1;
						}
						else
							fputs (buffer_in, file_kicad_mod_new);		//copy other lines without changes

					
					
					}
					//------------------------------------------------------
				
					fclose(file_kicad_mod_new);			
				}
				else
					perror ("Couldn't open the new .kicad_mod file");
				fclose(file_kicad_mod_old);	
			}
			else
				perror ("Couldn't open the old .kicad_mod file");
			
			// copy and rename 3d models
			//-----------------------------
			if (model_link_exists == 1)
			{
				for (k = 0; path_3d_old[k] != '\0'; ++k)		//if path contains backslash, change to slash to find file on linux
					if (path_3d_old[k] == '\\')
						path_3d_old[k] = '/';
				sprintf(old_3d_path,"./packages3d_bck/%s", path_3d_old);			
				sprintf(new_3d_path,"./packages3d/%.*s/%.*s.wrl", (unsigned int)(strlen(pretty_list[i].d_name) - strlen(".pretty")), pretty_list[i].d_name, (unsigned int)(strlen(footprint_list[j].d_name) - strlen(".kicad_mod")), footprint_list[j].d_name);
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
					model_link_exists = 2;
				}
				else
					fprintf(file_log_broken_link, "%s	%s	%s\n", pretty_list[i].d_name, footprint_list[j].d_name, old_3d_path);					
				sprintf(old_3d_path,"./packages3d_bck/%.*s.wings", (unsigned int)(strlen(path_3d_old) - strlen(".wrl")), path_3d_old);				
				sprintf(new_3d_path,"./packages3d/%.*s/%.*s.wings", (unsigned int)(strlen(pretty_list[i].d_name) - strlen(".pretty")), pretty_list[i].d_name, (unsigned int)(strlen(footprint_list[j].d_name) - strlen(".kicad_mod")), footprint_list[j].d_name);
				if((file_3d_old = fopen(old_3d_path, "rt")) != NULL)
				{
					if((file_3d_new = fopen(new_3d_path, "wt")) != NULL)
					{
						while((read_int = fgetc(file_3d_old)) != EOF)		//copy .wings even if it's not exported and link is broken
							fputc(read_int, file_3d_new);
						fclose(file_3d_new);	
					}	
					else
						perror ("Couldn't open the new .wings file");
					fclose(file_3d_old);
				}
				else
					if (model_link_exists == 2)		//check if .wrl exists
						fprintf(file_log_wrl_no_source, "%s\n", old_3d_path);		//write to list of .wrl without .wings
			}
			// END copy and rename 3d models
			//-----------------------------	
			printf("*");
			fflush(stdout);
		}
		printf("|\n");
		fflush(stdout);
	}
	fclose (file_log_copied_wrl);
	fclose (file_log_broken_link);


////////////////////////////////////////////////////////////////////////////
//////write paths to all .wrl and all .wings files to log files/////////////
	if ((G_file_log_all_wrl = fopen ("./log/all_wrl.log", "wt")) == NULL)
	{
		perror ("Couldn't open all_wrl.log file");
		exit (1);
	}
	if ((G_file_log_all_wings = fopen ("./log/all_wings.log", "wt")) == NULL)
	{
		perror ("Couldn't open all_wings.log file");
		exit (1);
	}
	if ((G_file_log_all_not_3d = fopen ("./log/all_not_3d.log", "wt")) == NULL)
	{
		perror ("Couldn't open all_not_3d.log file");
		exit (1);
	}
    ftw ("./packages3d_bck", search_wrl, 1);		//list all 3d .wrl files
    ftw ("./packages3d_bck", search_wings, 1);		//list all 3d .wings files
    ftw ("./packages3d_bck", search_not_3d, 1);		//list all not .wrl nor .wings files
	fclose (G_file_log_all_wrl);
	fclose (G_file_log_all_wings);
	fclose (G_file_log_all_not_3d);


/////////////////////////////////////////////////////////////////////////////////////////
///////// find unused .wrl and not exported .wings //////////////////////////////////////
	if ((G_file_log_all_wrl = fopen ("./log/all_wrl.log", "rt")) == NULL)
	{
		perror ("Couldn't open all_wrl.log file");
		exit (1);
	}
	if ((G_file_log_all_wings = fopen ("./log/all_wings.log", "rt")) == NULL)
	{
		perror ("Couldn't open all_wings.log file");
		exit (1);
	}
	if ((file_log_copied_wrl = fopen ("./log/copied_wrl.log", "rt")) == NULL)
	{
		perror ("Couldn't open copied_wrl.log file");
		exit (1);
	}
	if ((file_log_unused_wrl = fopen ("./log/unused_wrl.log", "wt")) == NULL)
	{
		perror ("Couldn't open unused_wrl.log file");
		exit (1);
	}
	if ((file_log_not_exported_wings = fopen ("./log/not_exported_wings.log", "wt")) == NULL)
	{
		perror ("Couldn't open not_exported_wings.log file");
		exit (1);
	}

	while (fgets (string_buffer_1, BUFFER_SIZE, G_file_log_all_wrl) != NULL)		//search unused .wrl files
	{
		flag = 0;
  		fseek (file_log_copied_wrl, 0, 0);
		while (fgets (string_buffer_2, BUFFER_SIZE, file_log_copied_wrl) != NULL)
			if (strcmp (string_buffer_1, string_buffer_2) == 0)	//check if file name from list of all .wrl files exists on list of copied .wrl files
				flag = 1;
		if (flag == 0)
			fprintf (file_log_unused_wrl, "%s", string_buffer_1);		//write to list of unused if not found in list of used
	}
  	fseek (G_file_log_all_wrl, 0, 0);
	while (fgets (string_buffer_1, BUFFER_SIZE, G_file_log_all_wings) != NULL)		//search not exported .wings files
	{
		flag = 0;
  		fseek (G_file_log_all_wrl, 0, 0);
		while (fgets (string_buffer_2, BUFFER_SIZE, G_file_log_all_wrl) != NULL)
			if (strncmp (string_buffer_1, string_buffer_2, strlen (string_buffer_1) - strlen (".wings")) == 0)	//check if file name (without ".wrl") from list of all .wings files exists on list of all .wrl
				flag = 1;
		if (flag == 0)
			fprintf (file_log_not_exported_wings, "%s",  string_buffer_1);	//write to list of not exported .wings files
	}
	fclose (file_log_not_exported_wings);
	fclose (G_file_log_all_wings);
	fclose (G_file_log_all_wrl);
	fclose (file_log_copied_wrl);
	fclose (file_log_unused_wrl);
	
////////////////////////////////////////////////////////////////////////////
////////// copy unused 3d models ///////////////////////////////////////////
	if ((file_log_unused_wrl = fopen ("./log/unused_wrl.log", "rt")) == NULL)
	{
		perror ("Couldn't open unused_wrl.log file");
		exit (1);
	}
		
	while (fgets (string_buffer_1, BUFFER_SIZE, file_log_unused_wrl) != NULL)		//get line with path of unused .wrl file
	{	
		strcpy (string_buffer_2, string_buffer_1);
		strcpy (old_3d_path, string_buffer_1);		//path to directory where unused .wrl is
		old_3d_path[strlen(string_buffer_1) - 1] = '\0';		//remove '\n'
		strcpy (new_3d_path, "./unused_3d/");
		for (k = 0; string_buffer_1[k] != '\0'; ++k)
			if (string_buffer_1[k] == '/')
				string_buffer_1[k] = '.';
		string_buffer_1[k - 1] = '\0';		//remove '\n'
		strcat (new_3d_path, string_buffer_1 + strlen("./packages3d_bck/"));

		if ((file_3d_old = fopen (old_3d_path, "rt")) != NULL)
		{			
			if ((file_3d_new = fopen (new_3d_path, "wt")) != NULL)		//copy
			{
				while ((read_char = fgetc (file_3d_old)) != EOF)
					fputc (read_char, file_3d_new);
				fclose (file_3d_new);
			}
			else
				perror ("Couldn't open the new .wrl file from list of unused");
			fclose (file_3d_old);
		}
		else
			perror ("Couldn't open the old .wrl file from list of unused");
			

		strcpy (old_3d_path, string_buffer_2);		//path to directory where unused .wings is
		old_3d_path[strlen (string_buffer_2) - strlen (".wrl") - 1] = '\0';		//remove ".wrl\n"
		strcat (old_3d_path, ".wings");
		strcat (new_3d_path, ".wings");		//append .wings
		if ((file_3d_old = fopen (old_3d_path, "rt")) != NULL)
		{
			if ((file_3d_new = fopen (new_3d_path, "wt")) != NULL)
			{
				while ((read_int = fgetc (file_3d_old)) != EOF)
					fputc (read_int, file_3d_new);
				fclose (file_3d_new);	
			}
			else
				printf ("Couldn't open the new .wings file, %s", string_buffer_2);
			fclose (file_3d_old);
		}
		else
			fprintf (file_log_wrl_no_source, "unused %s", string_buffer_2);	//write to list of .wrl without .wings

		printf(".");
		fflush(stdout); 
	}
	printf("done\n");
	fclose (file_log_unused_wrl);
	fclose (file_log_wrl_no_source);
	// END copy unused 3d models
	//-----------------------------		

	return 0;
}

