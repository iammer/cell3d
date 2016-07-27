/*   3D Cellular Automata
     (c)2002 Michael Ratliff
     Distributed under GNU Public License (See file gpl.txt)
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <memory.h>
#include <string.h>

#define true 1
#define false 0
#define NULL 0

/* envr and newr access the arrays env and new by index */
#define envr(X,Y,Z) (env[(X*arrsize.y+Y)*arrsize.z+Z])
#define newr(X,Y,Z) (new[(X*arrsize.y+Y)*arrsize.z+Z])
#define min(X,Y) ((X<Y) ? X : Y)

typedef int bool;

void initarr(unsigned char *,int); /*fills array with zeros*/
void randarr(unsigned char *,int); /*fills array with random values */
bool loadarr(unsigned char *,const char *); /*loads array from file*/
bool savearr(unsigned char *,const char *); /*saves array to file */
void glinit(void);
void display(void);
void reshape(int, int);
void key(unsigned char,int,int);
void idle(int); 
void onestep();  /* Does one cellular automata loop */
int count(int,int,int,char); /*Counts the number of inhabited cells adjacent
			       to another*/
void special(int,int,int); 
void info(unsigned char *);  /* Prints info about array */
int glprintf(char *,...);    /* Prints text to GL screen */
void removetxt(int);         /* Removes text from text buffer */
void showhelp(unsigned char,int,int);  /* Runs help menu */
bool makeenv(int,int,int);   /* Allocates memory for environment */
void freemem(void);          /* Frees memory from environment */
void swapenv(void);          /* Swaps env and new arrays */

unsigned char *env=NULL, *new=NULL; /*env is data for environment
				      new holds last data and is used by 
				      onestep */
int arrs,textsize=2048; /*arrs is array size, textsize is size of text
			  buffer */
char *text, *next;  /*text is the buffer for text output to screen
		      next is the next place to write to in that buffer*/
struct vector textpos;


struct vector {
    GLfloat x;
    GLfloat y;
};

/* 3D vector */
struct tdvector {
  GLfloat x;
  GLfloat y;
  GLfloat z;
};

struct {
  int x;
  int y;
  int z;
} arrsize;


/* The state of the environment */
struct {
  bool run;   /*Whether or not environment is running*/
  int interval; /*Interval between cycles */
  struct vector rot;  /*Current rotation of environment */
  struct vector rotmot; /*Current rotational motion of environment */
  GLfloat zoom;    
  char file[30];
  bool wall;       /* true if area outside environment is full
		      false if empty*/
  bool sphere;
  int lone;        /* lonely death rate */
  int crowd;       /* crowded death rate */
  int birth;       /* Birth value */
  int dtime;       /* Time in milliseconds to displat text */
  struct vector size; /* Size of GL window */
} state;

int main(int argc, char **argv) {
  char *file;  
  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
  glutInitWindowSize(640,480);
  glutInitWindowPosition(50,50);
  glutCreateWindow("3D Cellular Automata");
  glinit();
  
  state.run=false;
  state.interval=10;
  state.rot.x=5.0f;
  state.rot.y=-10.0f;
  state.zoom=1.0f;
  strcpy(state.file,"default.c3d");
  state.wall=false;
  state.birth=4;
  state.lone=3;
  state.crowd=6;
  state.dtime=5000;
  state.size.x=640;
  state.size.y=480;
  state.sphere=false;
  next=text=(char *)malloc(textsize);
  *next='\0';
  atexit(freemem);
  if (!makeenv(10,10,10)) {
    printf("Unable to allocate memory!\n");
    exit(1);
  }
  
  srand((unsigned int)time(NULL));
  
  glprintf("(c)2002 Michael Ratliff\nDistributed under the GNU Public License\n");
 
  if (loadarr(env,"cell3d.c3d")) {
  } else {
    glprintf("No initialization file found\n");
    initarr(env,arrs);
  }
 
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutTimerFunc(state.interval,idle,state.interval);
  glutKeyboardFunc(key);
  glutSpecialFunc(special);
  glutMainLoop();
  
  return 0;
}

void glinit(void) {
    glClearColor(0.0,0.0,0.0,0.0);
}

/* sets size bytes at arr to 0 */
void initarr(unsigned char *arr,int size) { 
  memset(arr,0,size);
}

/* sets size bytes at arr to 1 or 0 randomly */
void randarr(unsigned char *arr,int size) {
    int i;
    for (i=0;i<size;i++)
	arr[i]=(rand() < 1073741824) ? 1 : 0;
}

void reshape (int w,int h) {
  glViewport(0,0,(GLsizei)w,(GLsizei)h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0f,(GLfloat)w/(GLfloat)h,1.0f,100.0f);
  glMatrixMode(GL_MODELVIEW);
  state.size.x=w;
  state.size.y=h;
    
}


void key(unsigned char k,int x,int y) {
    int z;
    switch(k) {
    case 'q':
      exit(0);
      break;
    case 'c':
      initarr(env,arrs);
      display();
      break;
    case 'r':
      randarr(env,arrs);
      display();
      break;
    case 's':
      state.run=!state.run;
      if (state.run) {
	glprintf("Running...\n");
	if (state.rotmot.x==0 && state.rotmot.y==0)
	  idle(0);
      } else {
	glprintf("Stopped!\n");
      }
      break;
    case ',':
      state.interval*=2;
      glprintf("Interval is now %i.\n",state.interval);
      break;
    case '.':
      if (state.interval>1) state.interval/=2;
      glprintf("Interval is now %i.\n",state.interval);
      break;
    case 't':
      onestep();
      display();
      break;
    case 'p':
      state.sphere=!state.sphere;
      display();
      break;
    case '+':
      state.zoom*=1.5;
      glprintf("Zoom is now %f.\n",state.zoom);
      display();
      break;
    case '-':
      state.zoom/=1.5;
      glprintf("Zoom is now %f.\n",state.zoom);
      display();
      break;
    case 'b':
      swapenv();
      display();
      break;
    case 'l':
      if (loadarr(env,state.file)) {
	glprintf("Loaded from %s.\n",state.file);
	display();
      } else {
	glprintf("Error loading from %s.\n",state.file);
      }
      break;
    case 'v':
      if (savearr(env,state.file)) {
	glprintf("Wrote to %s.\n",state.file);
	display();
      } else {
	glprintf("Unable to write to %s.\n",state.file);
      }
      break;
    case 'f':
      printf("Current filename: %s.  Enter new filename: ",state.file);
      scanf("%s",state.file);
      printf("New filename is %s.\n");
      break;
    case 'z':
      printf("Current size: [%i,%i,%i]\nNew x dimension: ",arrsize.x,arrsize.y,arrsize.z);
      scanf("%i",&x);
      printf("New y dimension: ");
      scanf("%i",&y);
      printf("New z dimension: ");
      scanf("%i",&z);
      if (!makeenv(x,y,z)) {
	printf("Error reallocating memory!\n");
	exit(1);
      }
      break;
    case '8':
      if (state.rotmot.x==0 && state.rotmot.y==0 && !state.run) 
	glutTimerFunc(state.interval,idle,state.interval);
      state.rotmot.x+=5;
      break;
    case '2':
      if (state.rotmot.x==0 && state.rotmot.y==0 && !state.run)
	glutTimerFunc(state.interval,idle,state.interval);
      state.rotmot.x-=5;
      break;
    case '4':
      if (state.rotmot.x==0 && state.rotmot.y==0 && !state.run)
	glutTimerFunc(state.interval,idle,state.interval);
      state.rotmot.y-=5;
      break;
    case '6':
      if (state.rotmot.x==0 && state.rotmot.y==0 && !state.run)
	glutTimerFunc(state.interval,idle,state.interval);
      state.rotmot.y+=5;
      break;
    case '5':
      state.rotmot.x=state.rotmot.y=0;
      break;      
    case 'w':
      state.wall=!state.wall;
      glprintf("Wall is ");
      if (state.wall) {
	glprintf("full.\n");
      } else {
	glprintf("empty.\n");
      }
      break;
    case 127: /*delete key*/
      state.lone--;
      glprintf("Lonely death value is now: %i\n",state.lone);
      break;
    case 'i':
      info(env);
      break;
    case 'h':
      showhelp(0,0,0);
      break;
    }
    
}

void display(void) {
  int i,j,k;
  glClear(GL_COLOR_BUFFER_BIT);
  glLoadIdentity();
  gluLookAt(0.0f,0.0f,2.2f,0.0f,0.0f,0.0f,0.0f,1.0f,0.0f);

  /* Matrix stack stores position of beginging of last line*/
  if (text[0]!='\0') {
    glColor3f(0.0f,0.0f,1.0f);
    glPushMatrix();
    glTranslatef(textpos.x,textpos.y,0);
    glScalef(.0005,.0005,.0005);
    glPushMatrix();
    for (i=0;text[i]!='\0'; i++) {
      if (text[i]!='\n') {
	if (text[i]!='\t') {
	  glutStrokeCharacter(GLUT_STROKE_ROMAN, text[i]);
	} else {
	  glTranslatef(400,0,0);
	}
      } else {
	glPopMatrix();
	glTranslatef(0,-110,0);
	glPushMatrix();
      }
    }
    glPopMatrix();
    glPopMatrix();
  }
  
  if (state.rot.x>360) state.rot.x-=360;
  if (state.rot.y > 360) state.rot.y-=360;
  if (state.rot.x < 0 ) state.rot.x+=360;
  if (state.rot.y < 0 ) state.rot.y+=360;

  glRotatef(state.rot.x,1.0f,0.0f,0.0f);
  glRotatef(state.rot.y,0.0f,1.0f,0.0f);  

  glColor3f(1.0f,1.0f,1.0f);
  glutWireCube(state.zoom);

  glScalef(state.zoom/(GLfloat)arrsize.x,state.zoom/(GLfloat)arrsize.y,state.zoom/(GLfloat)arrsize.z);
  
  glColor4f(1.0f,0.0f,0.0f,0.2f);
  glTranslatef(-(GLfloat)arrsize.x/2.0f + 0.5f,-(GLfloat)arrsize.y/2 + 0.5f,-(GLfloat)arrsize.z/2.0f + 0.5f);
  
  /* Matrix stack stores position of last x axis plane and y axis line*/
  for(i=0;i<arrsize.x;i++) {
    glPushMatrix();
    
    /* Draw x-axis plane #i*/
    for(j=0;j<arrsize.y;j++) {
      glPushMatrix();
      
      /*Draw y-axis line #j in current x-axis plane */
      for(k=0;k<arrsize.z;k++) {
	if (envr(i,j,k)) {
	  if (state.sphere) {
	    glutWireSphere(0.5f,5,5);
	  } else {
	    glutWireCube(1.0f);
	  }
	}
	glTranslatef(0.0f,0.0f,1.0f); /*Step along z-axis*/
      }
      glPopMatrix();
      /* End draw y-axis line */
      
      glTranslatef(0.0f,1.0f,0.0f); /*Step along y-axis*/
    }
    glPopMatrix();
    /* End draw x-axis plane */
    
    glTranslatef(1.0f,0.0f,0.0f);  /*Step along x-axis*/
    }
  

  glutSwapBuffers();
}

void idle(int x) {
    if (state.run) {
      onestep();            
    }

      state.rot.x+=state.rotmot.x;
      state.rot.y+=state.rotmot.y;

    if (state.rotmot.x!=0 || state.rotmot.y!=0 || state.run) {
      display();
      glutTimerFunc(state.interval,idle,state.interval);
    }
}


/* Loads environment from file "file" to arr */
bool loadarr(unsigned char *arr,const char *file) {
    int val,i,x,y,z;
    FILE *fp;
    if (fp=fopen(file,"r")) {	
      initarr(arr,arrs);
      while(!feof(fp)) {
	    fscanf(fp,"%i,%i,%i %i",&x,&y,&z,&val);
	    if (x<arrsize.x && y<arrsize.y && z<arrsize.z) 
	      arr[(x*arrsize.y+y)*arrsize.z+z]=(unsigned char)val;
	}
	fclose(fp);
	return true;
    } else {
	return false;
    }
}

/* Writes environment from arr to file "file" */
bool savearr(unsigned char *arr,const char *file) {
  FILE *fp;
  int i,x,y,z,r;
  if (fp=fopen(file,"w")) {
    for(i=0;i<arrs;i++) {
      if (arr[i]!=0) {
	x=i/(arrsize.y*arrsize.z);
	r=i-x*arrsize.y*arrsize.z;
	y=r/arrsize.z;
	z=r-y*arrsize.z;
	fprintf(fp,"%i,%i,%i %i\n",x,y,z,(int)arr[i]);
      }
    }
    fclose(fp);
    return true;
  } else {
    return false;
  }
}


/* Does one cellular automata "generation" or cycle */
void onestep(void) {
  int i,j,k,c;
  for(i=0;i<arrsize.x;i++) {
    for(j=0;j<arrsize.y;j++) {
      for(k=0;k<arrsize.z;k++) {
	if (state.wall) {
	  c=26-count(i,j,k,0);
	} else {
	  c=count(i,j,k,1);
	}
	if (c<state.lone || c>state.crowd) {
	  newr(i,j,k)=0;
	} else if (c==state.birth) {
	  newr(i,j,k)=1;
	}
      }
    }
  }

  swapenv();
	  

}

/* returns the number of cells of type c surrounding (x,y,z)*/
int count(int x,int y,int z,char c) {
  int i,j,k,ret;
  struct tdvector l,u;

  l.x=(x==0)? 0 : (x-1);
  l.y=(y==0)? 0 : (y-1);
  l.z=(z==0)? 0 : (z-1);
  u.x=(x==arrsize.x-1)? arrsize.x-1 : (x+1);
  u.y=(y==arrsize.y-1)? arrsize.y-1 : (y+1);
  u.z=(z==arrsize.z-1)? arrsize.z-1 : (z+1);

  ret=0;
  for(i=l.x;i<=u.x;i++) {
    for(j=l.y;j<=u.y;j++) {
      for(k=l.z;k<=u.z;k++) {
	if (i!=x || j!=y || k!=z) {
	  if (c==envr(i,j,k)) ret++;
	}
      }
    }
  }
  return ret;
}

void special(int key,int x,int y) {
  switch(key) {
  case GLUT_KEY_UP:
    state.rot.x+=5;
    display();
    break;
  case GLUT_KEY_DOWN:
    state.rot.x-=5;
    display();
    break;
  case GLUT_KEY_LEFT:
    state.rot.y-=5;
    display();
    break;
  case GLUT_KEY_RIGHT:
    state.rot.y+=5;
    display();
    break;
  case GLUT_KEY_INSERT:
    state.lone++;
    glprintf("Lonely death value is now: %i\n",state.lone);
    break;
  case GLUT_KEY_HOME:
    state.crowd++;
    glprintf("Crowded death value is now: %i\n",state.crowd);
    break;
  case GLUT_KEY_END:
    state.crowd--;
    glprintf("Crowded death value is now: %i\n",state.crowd);
    break;
  case GLUT_KEY_PAGE_UP:
    state.birth++;
    glprintf("Birth value is now: %i\n",state.birth);
    break;
  case GLUT_KEY_PAGE_DOWN:
    state.birth--;
    glprintf("Birth value is now: %i\n",state.birth);
    break; 
  case GLUT_KEY_F1:
    showhelp(0,0,0);
    break;
  case GLUT_KEY_F2:
    if (!makeenv(10,10,10)) {
      printf("Error reallocating memory!\n");
      exit(1);
    }
    break;
  case GLUT_KEY_F3:
    if (!makeenv(15,15,15)) {
      printf("Error reallocating memory!\n");
      exit(1);
    }
    break;
  case GLUT_KEY_F4:
    if (!makeenv(20,20,20)) {
      printf("Error reallocating memory!\n");
      exit(1);
    }
    break;
  case GLUT_KEY_F5:
    if (!makeenv(25,25,25)) {
      printf("Error reallocating memory!\n");
      exit(1);
    }
    break;
  case GLUT_KEY_F6:
    if (!makeenv(30,30,30)) {
      printf("Error reallocating memory!\n");
      exit(1);
    }
    break;
  case GLUT_KEY_F7:
    if (!makeenv(35,35,35)) {
      printf("Error reallocating memory!\n");
      exit(1);
    }
    break;
  case GLUT_KEY_F8:
    if (!makeenv(40,40,40)) {
      printf("Error reallocating memory!\n");
      exit(1);
    }
    break;
  }
}

void info(unsigned char * arr) {
      int i,count[256];
      
      memset(count,0,256*sizeof(int));
      glprintf("Lonely death: %i    Crowded death: %i    Birth value: %i\n",state.lone,state.crowd,state.birth);
      glprintf("Environment is %ix%ix%i, total cells: %i\n",arrsize.x,arrsize.y,arrsize.z,arrs);
      for(i=0;i<arrs;i++)
	count[arr[i]]++;
      for(i=0;i<256;i++) {
	if (count[i]!=0) {
	  glprintf("There are %i cells with value: %i\n",count[i],i);
	}
      }
}


/* Prints the text passed to it (in printf style) to the GL screen 
   text is automatically removed after state.dtime milliseconds
   Returns the number of characters printed*/
int glprintf(char *format,...) {
  va_list args;
  int i,lines=0,ret;

  /* Copy text in args to text buffer */
  va_start(args, format);
  ret=vsprintf(next, format, args);
  va_end(args);

  /*Find number of carrage returns in text */
  for(i=0;text[i]!='\0';i++) {
    if (text[i]=='\n') lines++;
  }
  
  if (textsize-i < 1024) {
      textsize+=1024;
    if (text=(char *)realloc(text,textsize)) {
      next=&(text[i]);
    } else {
      printf("Unable to allocate memory for text buffer!\n");
      exit(1);
    }
  } else {
    next+=ret;
  }
  
    
  /* Position text so that last line is at the bottom of screen */
  textpos.x=-.9*state.size.x/state.size.y;
  textpos.y=-1+.055*(float)(lines+1);
  
  /* Schedule text to be removed */
  glutTimerFunc(state.dtime,removetxt,ret);
  display();
  return ret;
}


/* Removes n bytes of text from text buffer*/
void removetxt(int n) {
  int i,lines=0;
  for(i=0;text[i+n]!='\0';i++) {
    text[i]=text[i+n];
  }
  next-=n;
  *next='\0';
    for(i=0;text[i]!='\0';i++) {
    if (text[i]=='\n') lines++;
  }
  
  textpos.x=-.9*state.size.x/state.size.y;
  textpos.y=-1+.055*(float)(lines+1);
  display();
}

/*The keypress callback which generates the help system */
void showhelp(unsigned char k,int x,int y) {
  switch(k) {
  case 0:
    glutKeyboardFunc(showhelp);
  case 'h':
    glprintf("3D Cellular Automata\n   by Michael Ratliff\n");
    glprintf("Press q to quit\nTopics:\n(h)elp on help\n(c)ommands\n(l)icense\n(a)uthor\n");
    break;
  case 'a':
    glprintf("Written by Michael Ratliff\n\tratliffm@cis.ohio-state.edu\n");
    break;
  case 'l':
    glprintf("\
(c)2002 Michael Ratliff\n\
Distributed under the GNU Public License (see file gpl.txt)\n\
This program is free software; you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License as published by\n\
the Free Software Foundation; either version 2 of the License, or\n\
(at your option) any later version.\n\
\n\
This program is distributed in the hope that it will be useful,\n\
but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
GNU General Public License for more details.\n\
\n\
You should have received a copy of the GNU General Public License\n\
along with this program; if not, write to the Free Software\n\
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n");
    break;
  case 'q':
    glutKeyboardFunc(key);
    glprintf("You have quit help.\n");
    break;
  case 'c':
    glprintf("\
Commands:\n\
q - quit: Quit the program.\n\
c - clear: Clear the Environment\n\
r - Random: Fill the Environment with random cells\n\
s - Stop/Start: Toggle cellular automata action\n\
<,> - Change Speed: Change speed of automata\n\
t - Trace: Step through automata action cycle\n\
+,- Zoom:  Zoom in and out\n\
z - Size:  Change size of environment \n\t(Input valueys in terminal window)\n\
b - Back:  Toggle between last cycle and current cycle\n\
f - File:  Select name of current file \n\t(Input name in terminal window)\n\
v - Save:  Save to environment to current file\n\
l - Load:  Load environment from current file\n\
p - Sphere: Toggle cell shape (sphere/rect)\n\n\
8,2,4,6 (Numpad) - Continuous Rotation: Rotate environment \n\tcontinuously\n\
arrows - Rotate: Rotate environment\n\
w - Wall State: Toggle 'wall' ouside of environment as filled \n\tor empty\n\
i - Info: Information of state of environment\
Ins,Del - Lonely Death: Cells die when surrounded by less than \n\tthis many others\n\
Home,End - Crowded Death: Cells die when surrounded by more than \n\tthis many others\n\
PgUp,PgDown - Birth value: Cells are born when surrounded by \n\texactly this many others\n\
F2-F7 - Size Preset: Changes enironment size to prest values\n");
    break;
  default:
    glprintf("**You are still in help.  Press q to exit.**\n");
  }
}

void freemem(void) {
  if (env) free(env);
  if (new) free(new);
  if (text) free(text);
  printf("Exiting Gracefully :)\n");
}

void swapenv(void) {
  unsigned char *tmp;
  tmp=env;
  env=new;
  new=tmp;
}

/*Returns false when unable to allocate memory*/
bool makeenv(int x,int y,int z) {
  int i,j,k;
  arrs=x*y*z;
  swapenv();
  env=(unsigned char *)realloc(env,arrs);
  initarr(env,arrs);
  for(i=0;i<min(arrsize.x,x);i++) {
    for(j=0;j<min(arrsize.y,y);j++) {
      for(k=0;k<min(arrsize.z,z);k++) {
	env[(i*y+j)*z+k]=newr(i,j,k);
      }
    }
  }
  new=(unsigned char *)realloc(new,arrs);
  arrsize.x=x;
  arrsize.y=y;
  arrsize.z=z;
  display();

  return (env && new);

}
