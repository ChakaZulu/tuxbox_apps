#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "lcd-ks0713.h"

/*
  012
  3D4 (D: Dennis)
  567

*/

/*
typedef unsigned char screen_t[LCD_BUFFER_SIZE];

void draw_screen(int lcd_fd, screen_t s) {
  write(lcd_fd, s, LCD_BUFFER_SIZE);
}

void putpixel(int x, int y, char col, screen_t s) {
  int ofs = (y>>3)*LCD_COLS+x,
    bit = y & 7;
  
  switch (col) {
  case LCD_PIXEL_ON:
    s[ofs] |= 1<<bit;
    break;
  case LCD_PIXEL_OFF:
    s[ofs] &= ~(1<<bit);
    break;
  case LCD_PIXEL_INV:
    s[ofs] ^= 1<<bit;
    break;
  }
}
*/

int main(int argc, char **argv) {
  unsigned char buf;
  int lcdfd,rndfd;
  int i,delay=1000;
  //  screen_t screen;
  lcd_pixel pixl;

  //  memset(screen,0,sizeof(screen));

  if(argc>1)
    delay=atoi(argv[1]);

  lcdfd=open("/dev/dbox/lcd0",O_RDWR);
  rndfd=open("/dev/urandom",O_RDONLY);
  
  i=LCD_MODE_ASC;
  ioctl(lcdfd,LCD_IOCTL_ASC_MODE,&i);
  ioctl(lcdfd,LCD_IOCTL_CLEAR);
  write(lcdfd,"\nLet's see if\ndrunken Dennis\ncan find his\nway home...\n\n\n",strlen("\nLet's see if\ndrunken Dennis\ncan find his\nway home...\n\n\n"));
  sleep(5);
  i=LCD_MODE_BIN;
  ioctl(lcdfd,LCD_IOCTL_ASC_MODE,&i);
  ioctl(lcdfd,LCD_IOCTL_CLEAR);
  read(rndfd,&buf,1);
  pixl.x=buf%119;
  read(rndfd,&buf,1);
  pixl.y=buf%63;
  pixl.v=2;
  while(read(rndfd,&buf,1)) {
    buf%=8;
    switch(buf) {
    case 0:
      if ((pixl.x==0) || (pixl.y==0)) break;
      pixl.x--;
      pixl.y--;
      break;
    case 1:
      if(pixl.y==0) break;
      pixl.y--;
      break;
    case 2:
      if((pixl.x==119) || (pixl.y==0)) break;
      pixl.x++;
      pixl.y--;
      break;
    case 3:
      if(pixl.x==0) break;
      pixl.x--;
      break;
    case 4:
      if(pixl.x==119) break;
      pixl.x++;
      break;
    case 5:
      if((pixl.x==0) || (pixl.y==63)) break;
      pixl.x--;
      pixl.y++;
      break;
    case 6:
      if(pixl.y==63) break;
      pixl.y++;
      break;
    case 7:
      if((pixl.x==119) || (pixl.y==63)) break;
      pixl.x++;
      pixl.y++;
    }
    ioctl(lcdfd,LCD_IOCTL_SET_PIXEL,&pixl);
    /*
      putpixel(pixl.x,pixl.y,pixl.v,screen);
      draw_screen(lcdfd,screen);
    */
    usleep(delay);
  }
  i=LCD_MODE_ASC;
  ioctl(lcdfd,LCD_IOCTL_ASC_MODE,&i);
  ioctl(lcdfd,LCD_IOCTL_CLEAR);
  close(rndfd);
  close(lcdfd);
  return 0;
}
