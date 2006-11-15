/******************************* LICENCE **************************************
* Any code in this file may be redistributed or modified under the terms of
* the GNU General Public Licence as published by the Free Software 
* Foundation; version 2 of the licence.
****************************** END LICENCE ***********************************/

/******************************************************************************
* Author:
* Andrew Smith, http://littlesvr.ca/misc/contactandrew.php
*
* Contributors:
* The Samba Project - http://samba.org/
* - most of the filename mangling code
******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

#include "bk.h"
#include "bkInternal.h"
#include "bkMangle.h"
#include "bkError.h"

/* length of aaa in aaa~xxxx.bbb */
#define NCHARS_9660_BASE 3

/*
* note that some unsigned ints in mangling functions are
* required to be 32 bits long for the hashing to work
* see the samba code for details
*/

/******************************************************************************
* charIsValid9660()
* 
* */
bool charIsValid9660(char theChar)
{
    if( (theChar >= '0' && theChar <= '9') ||
        (theChar >= 'a' && theChar <= 'z') ||
        (theChar >= 'A' && theChar <= 'Z') ||
        strchr("_-$~", theChar) )
    {
        return true;
    }
    else
        return false;
}

/******************************************************************************
* charIsValidJoliet()
* 
* */
bool charIsValidJoliet(char theChar)
{
    if( (theChar >= '0' && theChar <= '9') ||
        (theChar >= 'a' && theChar <= 'z') ||
        (theChar >= 'A' && theChar <= 'Z') ||
        strchr("_-$~.", theChar) )
    {
        return true;
    }
    else
        return false;
}

/* 
   hash a string of the specified length. The string does not need to be
   null terminated 

   this hash needs to be fast with a low collision rate (what hash doesn't?)
*/
unsigned hashString(const char *str, unsigned int length)
{
    unsigned value;
    unsigned i;
    
    static const unsigned fnv1Prime = 0x01000193;
    
    /* Set the initial value from the key size. */
    /* fnv1 of the string: idra@samba.org 2002 */
    value = 0xa6b93095;
    for (i = 0; i < length; i++)
    {
        value *= (unsigned)fnv1Prime;
        value ^= (unsigned)(str[i]);
    }
    
    /* note that we force it to a 31 bit hash, to keep within the limits
       of the 36^6 mangle space */
    return value & ~0x80000000;  
}

/******************************************************************************
* mangleDir()
* Mangles the filenames from origDir and puts the results into newDir, whcich
* it also creates.
* filenameTypes is all types required in the end
* */
int mangleDir(const Dir* origDir, DirToWrite* newDir, int filenameTypes)
{
    int rc;
    bool haveCollisions;
    int numTimesTried;
    int num9660Collisions;
    char newName9660[13]; /* for remangling */
    int numJolietCollisions;
    char newNameJoliet[NCHARS_FILE_ID_MAX_JOLIET]; /* for remangling */
    
    DirLL* currentOrigDir;
    DirToWriteLL** currentNewDir;
    FileLL* currentOrigFile;
    FileToWriteLL** currentNewFile;
    
    DirToWriteLL* currentDir;
    FileToWriteLL* currentFile;
    DirToWriteLL* currentDirToCompare;
    FileToWriteLL* currentFileToCompare;
    
    /* MANGLE all names, create new dir/file lists */
    currentOrigDir = origDir->directories;
    currentNewDir = &(newDir->directories);
    while(currentOrigDir != NULL)
    /* have directories */
    {
        *currentNewDir = malloc(sizeof(DirToWriteLL));
        if(*currentNewDir == NULL)
        {
            newDir->files = NULL;
            return BKERROR_OUT_OF_MEMORY;
        }
        
        bzero(*currentNewDir, sizeof(DirToWriteLL));
        
        shortenNameFor9660(currentOrigDir->dir.name, 
                          (*currentNewDir)->dir.name9660, true);
        
        if(filenameTypes | FNTYPE_ROCKRIDGE)
            strcpy((*currentNewDir)->dir.nameRock, currentOrigDir->dir.name);
        else
            (*currentNewDir)->dir.nameRock[0] = '\0';
        
        if(filenameTypes | FNTYPE_JOLIET)
            //strcpy((*currentNewDir)->dir.nameJoliet, currentOrigDir->dir.name);
            mangleName(currentOrigDir->dir.name, (*currentNewDir)->dir.nameJoliet, false);
        else
            (*currentNewDir)->dir.nameJoliet[0] = '\0';
        
        (*currentNewDir)->dir.posixFileMode = currentOrigDir->dir.posixFileMode;
        
        rc = mangleDir(&(currentOrigDir->dir), &((*currentNewDir)->dir), 
                       filenameTypes);
        if(rc < 0)
        {
            newDir->files = NULL;
            free(*currentNewDir);
            *currentNewDir = NULL;
            return rc;
        }
        
        currentOrigDir = currentOrigDir->next;
        
        currentNewDir = &((*currentNewDir)->next);
    }
    
    currentOrigFile = origDir->files;
    currentNewFile = &(newDir->files);
    while(currentOrigFile != NULL)
    /* have files */
    {
        *currentNewFile = malloc(sizeof(FileToWriteLL));
        if(*currentNewFile == NULL)
            return BKERROR_OUT_OF_MEMORY;
        
        bzero(*currentNewFile, sizeof(FileToWriteLL));
        
        shortenNameFor9660(currentOrigFile->file.name, 
                          (*currentNewFile)->file.name9660, false);
        
        if(filenameTypes | FNTYPE_ROCKRIDGE)
            strcpy((*currentNewFile)->file.nameRock, currentOrigFile->file.name);
        else
            (*currentNewFile)->file.nameRock[0] = '\0';
        
        if(filenameTypes | FNTYPE_JOLIET)
            //strcpy((*currentNewFile)->file.nameJoliet, currentOrigFile->file.name);
            mangleName(currentOrigFile->file.name, (*currentNewFile)->file.nameJoliet, false);
        else
            (*currentNewFile)->file.nameJoliet[0] = '\0';
        
        (*currentNewFile)->file.posixFileMode = currentOrigFile->file.posixFileMode;
        
        (*currentNewFile)->file.size = currentOrigFile->file.size;
        
        (*currentNewFile)->file.onImage = currentOrigFile->file.onImage;
        
        (*currentNewFile)->file.offset = currentOrigFile->file.position;
        
        if( !currentOrigFile->file.onImage )
        {
            (*currentNewFile)->file.pathAndName = malloc(strlen(currentOrigFile->file.pathAndName) + 1);
            if( (*currentNewFile)->file.pathAndName == NULL )
            {
                free(*currentNewFile);
                *currentNewFile = NULL;
                return BKERROR_OUT_OF_MEMORY;
            }
            
            strcpy((*currentNewFile)->file.pathAndName, currentOrigFile->file.pathAndName);
        }
        
        (*currentNewFile)->file.origFile = &(currentOrigFile->file);
        
        currentOrigFile = currentOrigFile->next;
        
        currentNewFile = &((*currentNewFile)->next);
    }
    /* END MANGLE all names, create new dir/file lists */
    
    haveCollisions = true;
    numTimesTried = 0;
    while(haveCollisions && numTimesTried < 500) /* random big number */
    {
        haveCollisions = false;
        
        // for each subdir
          // look through entire dir list and count collisions
          // look through entire file list and count collisions
          // if more than 1, remangle name
        
        // for each file
          // look through entire dir list and count collisions
          // look through entire file list and count collisions
          // if more than 1, remangle name
        
        currentDir = newDir->directories;
        while(currentDir != NULL)
        {
            num9660Collisions = 0;
            numJolietCollisions = 0;
            
            currentDirToCompare = newDir->directories;
            while(currentDirToCompare != NULL)
            {
                if(strcmp(currentDir->dir.name9660, 
                          currentDirToCompare->dir.name9660) == 0)
                {
                    num9660Collisions++;
                }
                
                if(strcmp(currentDir->dir.nameJoliet, 
                          currentDirToCompare->dir.nameJoliet) == 0)
                {
                    numJolietCollisions++;
                }
                
                currentDirToCompare = currentDirToCompare->next;
            }
            
            currentFileToCompare = newDir->files;
            while(currentFileToCompare != NULL)
            {
                if(strcmp(currentDir->dir.name9660, 
                          currentFileToCompare->file.name9660) == 0)
                {
                    num9660Collisions++;
                }
                
                if(strcmp(currentDir->dir.nameJoliet, 
                          currentFileToCompare->file.nameJoliet) == 0)
                {
                    numJolietCollisions++;
                }
                
                currentFileToCompare = currentFileToCompare->next;
            }
            
            if(num9660Collisions != 1)
            {
                haveCollisions = true;
                
                mangleNameFor9660(currentDir->dir.name9660, newName9660, true);
                
                strcpy(currentDir->dir.name9660, newName9660);
            }
            
            if(numJolietCollisions != 1)
            {
                haveCollisions = true;
                
                mangleName(currentDir->dir.nameJoliet, newNameJoliet, true);
                
                strcpy(currentDir->dir.nameJoliet, newNameJoliet);
            }
            
            currentDir = currentDir->next;
        }
        
        currentFile = newDir->files;
        while(currentFile != NULL)
        {
            num9660Collisions = 0;
            numJolietCollisions = 0;
            
            currentDirToCompare = newDir->directories;
            while(currentDirToCompare != NULL)
            {
                if(strcmp(currentFile->file.name9660, 
                          currentDirToCompare->dir.name9660) == 0)
                {
                    num9660Collisions++;
                }
                
                if(strcmp(currentFile->file.nameJoliet, 
                          currentDirToCompare->dir.nameJoliet) == 0)
                {
                    numJolietCollisions++;
                }
                
                currentDirToCompare = currentDirToCompare->next;
            }
            
            currentFileToCompare = newDir->files;
            while(currentFileToCompare != NULL)
            {
                if(strcmp(currentFile->file.name9660, 
                          currentFileToCompare->file.name9660) == 0)
                {
                    num9660Collisions++;
                }
                
                if(strcmp(currentFile->file.nameJoliet, 
                          currentFileToCompare->file.nameJoliet) == 0)
                {
                    numJolietCollisions++;
                }
                
                currentFileToCompare = currentFileToCompare->next;
            }
            
            if(num9660Collisions != 1)
            {
                haveCollisions = true;
                
                mangleNameFor9660(currentFile->file.name9660, newName9660, false);
                
                strcpy(currentFile->file.name9660, newName9660);
            }
            
            if(numJolietCollisions != 1)
            {
                haveCollisions = true;
                
                mangleName(currentFile->file.nameJoliet, newNameJoliet, true);
                
                strcpy(currentFile->file.nameJoliet, newNameJoliet);
            }
            
            currentFile = currentFile->next;
        }
        
        numTimesTried++;
    }
    
    if(haveCollisions)
        return BKERROR_MANGLE_TOO_MANY_COL;
    
    return 1;
}

/******************************************************************************
* mangleNameFor9660()
* Convert a long filename into an ISO9660 acceptable form: 
* all uppercase + digits + '_' + "~", 8 chars max for directories and 8.3 chars
* for files. Extension is kept if it's shorter then 4 chars.
* 3 chars from the original name are kept, the rest is filled with ~XXXX where
* the XXXX is a random string (but still with valid characters).
* */
void mangleNameFor9660(const char* origName, char* newName, bool isADir)
{
    char* dot_p;
    int i;
    char base[7]; /* max 6 chars */
    char extension[4]; /* max 3 chars */
    int extensionLen;
    unsigned hash;
    unsigned v;
    /* these are the characters we use in the 8.3 hash. Must be 36 chars long */
    static const char* baseChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    
    /* FIND extension */
    if(isADir)
    {
        dot_p = NULL;
    }
    else
    {
        dot_p = strrchr(origName, '.');
        
        if(dot_p)
        {
            /* if the extension contains any illegal characters or
               is too long (> 3) or zero length then we treat it as part
               of the prefix */
            for(i = 0; i < 4 && dot_p[i + 1] != '\0'; i++)
            {
                if( !charIsValid9660(dot_p[i + 1]) )
                {
                    dot_p = NULL;
                    break;
                }
            }
            
            if(i == 0 || i == 4 || dot_p == origName)
                dot_p = NULL;
        }
    }
    /* END FIND extension */
    
    /* GET base */
    /* the leading characters in the mangled name is taken from
    *  the first characters of the name, if they are ascii otherwise
    *  '_' is used */
    for(i = 0; i < NCHARS_9660_BASE && origName[i] != '\0'; i++)
    {
        base[i] = origName[i];
        
        if ( !charIsValid9660(origName[i]) )
            base[i] = '_';
        
        base[i] = toupper(base[i]);
    }
    
    /* make sure base doesn't contain part of the extension */
    if(dot_p != NULL)
    {
        //!! test this
        if(i > dot_p - origName)
            i = dot_p - origName;
    }
    
    /* fixed length */
    while(i < NCHARS_9660_BASE)
    {
        base[i] = '_';
        
        i++;
    }
    
    base[NCHARS_9660_BASE] = '\0';
    /* END GET base */
    
    /* GET extension */
    /* the extension of the mangled name is taken from the first 3
       ascii chars after the dot */
    extensionLen = 0;
    if(dot_p)
    {
        for(i = 1; extensionLen < 3 && dot_p[i] != '\0'; i++)
        {
            extension[extensionLen] = toupper(dot_p[i]);
            
            extensionLen++;
        }
    }
    
    extension[extensionLen] = '\0';
    /* END GET extension */
    
    /* find the hash for this prefix */
    hash = hashString(origName, strlen(origName));
    
    /* now form the mangled name. */
    for(i = 0; i < NCHARS_9660_BASE; i++)
    {
        newName[i] = base[i];
    }
    
    newName[NCHARS_9660_BASE] = '~';
    
    v = hash;
    newName[7] = baseChars[v % 36];
    for(i = 6; i > NCHARS_9660_BASE; i--)
    {
        v = v / 36;
        newName[i] = baseChars[v % 36];
    }
    
    /* add the extension and terminate string */
    if(extensionLen > 0)
    {
        newName[8] = '.';
        
        strcpy(newName + 9, extension);
    }
    else
    {
        newName[8] = '\0';
    }
    
    printf("remangled '%s' -> '%s'\n", origName, newName);
}

void mangleName(const char* origName, char* newName, bool appendHash)
{
    char* dot_p;
    int i;
    char base[NCHARS_FILE_ID_MAX_JOLIET]; /* '\0' terminated */
    char extension[6]; /* max 3 chars */
    int extensionLen;
    unsigned hash;
    unsigned v;
    char hashStr[5]; /* '\0' terminated */
    /* these are the characters we use in the 8.3 hash. Must be 36 chars long */
    static const char* baseChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    printf("mangling '%s'\n", origName);
    /* FIND extension candidate */
    dot_p = strrchr(origName, '.');
    
    if(dot_p)
    {
        /* if the extension contains any illegal characters or
           is too long (> 5) or zero length then we treat it as part
           of the prefix */
        for(i = 0; i < 6 && dot_p[i + 1] != '\0'; i++)
        {
            if( !charIsValidJoliet(dot_p[i + 1]) )
            {
                dot_p = NULL;
                break;
            }
        }
        
        if(i == 0 || i == 6 || dot_p == origName)
            dot_p = NULL;
    }
    /* END FIND extension candidate */
    printf("dot_p %d\n", dot_p);
    /* GET base */
    /* The leading characters in the mangled name are taken from
    *  the first characters of the name if they are allowed, otherwise
    *  '_' is used */
    for(i = 0; i < NCHARS_FILE_ID_MAX_JOLIET - 1 && origName[i] != '\0'; i++)
    {
        base[i] = origName[i];
        
        if ( !charIsValidJoliet(origName[i]) )
            base[i] = '_';
    }
    printf("i %d\n", i);
    /* make sure base doesn't contain part of the extension */
    if(dot_p != NULL)
    {
        if(i > dot_p - origName)
            i = dot_p - origName;
    }
    printf("i %d\n", i);
    base[i] = '\0';
    /* END GET base */
    printf("base: '%s'\n", base);
    /* GET extension */
    /* the extension of the mangled name is taken from the first 3
       ascii chars after the dot */
    extensionLen = 0;
    if(dot_p)
    {
        for(i = 1; extensionLen < 5 && dot_p[i] != '\0'; i++)
        {
            extension[extensionLen] = dot_p[i];
            
            extensionLen++;
        }
    }
    
    extension[extensionLen] = '\0';
    /* END GET extension */
    printf("extension: '%s'\n", extension);fflush(NULL);
    
    /* FIND the hash for this prefix */
    hash = hashString(origName, strlen(origName));
    
    hashStr[4] = '\0';
    v = hash;
    hashStr[3] = baseChars[v % 36];
    for(i = 2; i >= 0; i--)
    {
        v = v / 36;
        hashStr[i] = baseChars[v % 36];
    }
    /* END FIND the hash for this prefix */
    printf("hash: '%s'\n", hashStr);fflush(NULL);
    
    /* ASSEMBLE name */
    strcpy(newName, base);
    printf("new base: '%s', limit %d\n", newName, NCHARS_FILE_ID_MAX_JOLIET - 1 - 4 - 1 - 5);
    if(appendHash)
    {
        /* max name len - '~' - hash - '.' - extension */
        if(strlen(newName) >= NCHARS_FILE_ID_MAX_JOLIET - 1 - 1 - 4 - 1 - 5)
            newName[NCHARS_FILE_ID_MAX_JOLIET - 1 - 1 - 4 - 1 - 5] = '\0';
        
        strcat(newName, "~");
        strcat(newName, hashStr);
    }
    if(extensionLen > 0)
    {
        strcat(newName, ".");
        strcat(newName, extension);
    }
    /* END ASSEMBLE name */
    
    printf("joliet mangle '%s', len %d\n", newName, strlen(newName));
}

/******************************************************************************
* shortenNameFor9660()
* Same as mangleNameFor9660() but without the ~XXXX.
* */
void shortenNameFor9660(const char* origName, char* newName, bool isADir)
{
    char* dot_p;
    int i;
    char base[9]; /* max 9 chars */
    char extension[4]; /* max 3 chars */
    int extensionLen;
    
    /* FIND extension */
    /* ISO9660 requires that directories have no dots ('.') but some isolinux 
    * cds have the kernel in a directory with a dot so i need to allow dots in
    * directories :( */
    /*if(isADir)
    {
        dot_p = NULL;
    }
    else
    {*/
        dot_p = strrchr(origName, '.');
        
        if(dot_p)
        {
            /* if the extension contains any illegal characters or
               is too long (> 3) or zero length then we treat it as part
               of the prefix */
            for(i = 0; i < 4 && dot_p[i + 1] != '\0'; i++)
            {
                if( !charIsValid9660(dot_p[i + 1]) )
                {
                    dot_p = NULL;
                    break;
                }
            }
            
            if(i == 0 || i == 4 || dot_p == origName)
                dot_p = NULL;
        }
    /*}*/
    /* END FIND extension */
    
    /* GET base */
    /* the leading characters in the mangled name is taken from
    *  the first characters of the name, if they are allowed otherwise
    *  '_' is used */
    for(i = 0; i < 8 && origName[i] != '\0'; i++)
    {
        base[i] = origName[i];
        
        if ( !charIsValid9660(origName[i]) )
            base[i] = '_';
        
        base[i] = toupper(base[i]);
    }
    
    /* make sure base doesn't contain part of the extension */
    if(dot_p != NULL)
    {
        //!! test this to make sure it works
        if(i > dot_p - origName)
            i = dot_p - origName;
    }
    
    base[i] = '\0';
    /* END GET base */
    
    /* GET extension */
    /* the extension of the mangled name is taken from the first 3
       ascii chars after the dot */
    extensionLen = 0;
    if(dot_p)
    {
        for(i = 1; extensionLen < 3 && dot_p[i] != '\0'; i++)
        {
            extension[extensionLen] = toupper(dot_p[i]);
            
            extensionLen++;
        }
    }
    
    extension[extensionLen] = '\0';
    /* END GET extension */
    
    strcpy(newName, base);
    if(extensionLen > 0)
    {
        strcat(newName, ".");
        strcat(newName, extension);
    }
}
