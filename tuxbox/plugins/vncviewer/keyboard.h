#ifndef _KEYBOARD_H
#define _KEYBOARD_H

struct hbtn {
        int shift_l;
        int shift_r;
        int altgr;
        int mouse1;
        int mouse2;
        int mouse3;
        int pan;
        int action;
};

#define N_SCANCODE 256
extern int *fbvnc_keymap;
extern struct hbtn hbtn;
extern bool img_saved;
extern bool fn_action;
extern int pan_toggle_count;
extern int rep_key;
extern char btn_state[N_SCANCODE];

extern void init_keyboard(void);
extern bool key_special_action(int key);
extern int /* keysym */ key_map(int hwkey);
extern void key_press(int hwkey);
extern void key_release(int hwkey);

#endif
