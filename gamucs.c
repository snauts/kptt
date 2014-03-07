#include "bat.h"
#include "spin.h"
#include "title.h"
#include "mouse.h"
#include "button.h"
#include "tunnel.h"
#include "tables.h"
#include "cheese.h"

#define OPTIMIZE_ASM 1

unsigned short mouse_x;
unsigned char  mouse_y;
unsigned char  mouse_frame;
signed char    mouse_dir;

unsigned char frame = 0;

unsigned char note_delay = 0;
unsigned char music_index = 0;
unsigned char note_release = 1;
unsigned char voice_channel = 0;

unsigned char wave_form_on[] = { 0x41, 0x81, 0x41 };
unsigned char wave_form_off[] = { 0x40, 0x80, 0x40 };

#define MOUSE_SPIN	-2
#define MOUSE_START	24

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#define MEM(address) (* (unsigned char *) (address))

#define PTR(address) ((unsigned char *) (address))

#define LINE(nr) (40 * (nr)) // 40 bytes per scanline

#define KEY_DOWN(key) (KEY_##key)

unsigned char KEY_X = 0;
unsigned char KEY_Z = 0;

unsigned char KEY_RET = 0;
unsigned char KEY_ESC = 0;

#define BORDER_COLOR(c) \
    MEM(0xd020) = (c);

#define BACKGROUND_COLOR(c) \
    MEM(0xd021) = (c);

#define COLOR_BLACK	0x0
#define COLOR_D_RED	0x2
#define COLOR_D_GREEN	0x5
#define COLOR_YELLOW	0x7
#define COLOR_ORANGE	0x8
#define COLOR_BROWN	0x9
#define COLOR_L_RED	0xa
#define COLOR_D_GREY	0xb
#define COLOR_GREY	0xc
#define COLOR_L_GREEN	0xd
#define COLOR_L_GREY	0xf

#define COLOR(c1, c2) ((COLOR_##c1 << 4) | COLOR_##c2)

#define SWITCH_VIC_BANK() \
    MEM(0xdd00) = (MEM(0xdd00) & ~3) | 1;

#define MULTI_COLOR_BITMAP_MODE() \
    MEM(0xd011) = 0x3b; \
    MEM(0xd016) = 0xd8; \
    MEM(0xd018) = 0x38;

#define SPRITE_COMMON_COLORS_GREY() \
    MEM(0xd025) = COLOR_L_GREY; \
    MEM(0xd026) = COLOR_D_GREY;

#define USE_SPRITE_DOUBLE_WIDTH() \
    MEM(0xd01c) = 0xff;

#define SPRITE_COLLISION() MEM(0xd01e)

#define MOUSE_HIT() (SPRITE_COLLISION() & 1)

#define INIT_SID() \
	memset(PTR(0xd400), 0, 28); \
	MEM(0xd418) = 0xf;

#define DISABLE_INTERRUPTS() \
    __asm__ ("sei"); \
    MEM(0xdc02) = 0xff; \
    MEM(0xdc03) = 0x00; \

#define CFG_SID(channel, pw_lo, pw_hi, atk_dcy, stn_rls, form_on, form_off) \
    MEM(0xd402 + 7 * channel) = pw_lo;   \
    MEM(0xd403 + 7 * channel) = pw_hi;   \
    MEM(0xd405 + 7 * channel) = atk_dcy; \
    MEM(0xd406 + 7 * channel) = stn_rls; \
    wave_form_off[channel] = form_off;   \
    wave_form_on[channel] = form_on;

static void wait_ray(unsigned char test) {
    while ((MEM(0xd011) & 0x80) == test) { }
}

static void check_keys(void) {
    static char key_states = 0;

    MEM(0xdc02) = 0xe0;

    key_states = ~MEM(0xdc00) | ~MEM(0xdc01);
    KEY_Z = key_states & 0x04;
    KEY_X = key_states & 0x08;

    MEM(0xdc02) = 0xff;

    MEM(0xdc00) = ~0x02;
    KEY_Z |= ~MEM(0xdc01) & 0x10;
    MEM(0xdc00) = ~0x04;
    KEY_X |= ~MEM(0xdc01) & 0x80;
    MEM(0xdc00) = ~0x01;
    KEY_RET = ~MEM(0xdc01) & 0x02;
    MEM(0xdc00) = ~0x80;
    KEY_ESC = ~MEM(0xdc01) & 0x80;
}

static void wait_frame_finish(void) {
    check_keys();
    wait_ray(0x80);
    wait_ray(0x00);
}

void* __fastcall__ memset(void *ptr, int c, int n);
void* __fastcall__ memcpy(void *dest, const void *src, int n);

static void play_music(void);

static void decompress(unsigned char *dst, unsigned char *src) {
    while (*src) {
	unsigned char info = *(src++);
	unsigned char count = info & 0x7f;
	unsigned char advance = info & 0x80;
	if (advance) {
	    memcpy(dst, src, count);
	    src += count;
	}
	else {
	    memset(dst, *src, count);
	    src += 1;
	}
	dst += count;
    }
}

unsigned char shift_bits[] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
};

unsigned char mask_bits[] = {
    0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f,
};

#if OPTIMIZE_ASM
extern void set_sprite_pos(char num, unsigned short x, unsigned char y);
#else
static void set_sprite_pos(char num, unsigned short x, unsigned char y) {
    if (x & 0x100) {
	MEM(0xd010) |=  shift_bits[num];
    }
    else {
	MEM(0xd010) &= ~shift_bits[num];
    }
    num = num << 1;
    MEM(0xd000 + num) = x;
    MEM(0xd001 + num) = y;
}
#endif

#define SPRITE_COLOR(num, color) \
    MEM(0xd027 + num) = (color);

#define SPRITE_PTR(num, ptr) \
    MEM(0x8ff8 + num) = (ptr);

#define SPRITE_ENABLE(mask) \
    MEM(0xd015) = (mask);

static void button_switch(char b1, char b2) {
    set_sprite_pos(1, 159, b1 ? 182 : 180);
    set_sprite_pos(2, 185, b2 ? 182 : 180);
    SPRITE_PTR(3, b1 ? 0x1 : 0x0);
    SPRITE_PTR(4, b2 ? 0x1 : 0x0);
}

#if OPTIMIZE_ASM
extern unsigned char get_mouse_ptr(void);
#else
static unsigned char get_mouse_ptr(void) {
    switch (mouse_dir) {
    case 0:
	return 0x10;
    case MOUSE_SPIN:
	return 0x10 + ((mouse_x >> 3) & 7);
    default:
	return 0x08 + (((mouse_x + mouse_frame) >> 2) & 7);
    }
}
#endif

static void animate_mouse(void) {
    set_sprite_pos(0, mouse_x, mouse_y);
    SPRITE_PTR(0, get_mouse_ptr());
}

static void punch(void) {
    MEM(0xd407) = 0x00;
    MEM(0xd408) = 0x10;
    MEM(0xd40c) = 0x88;
    MEM(0xd40d) = 0x88;
    MEM(0xd40b) = 0x81;
    MEM(0xd40b) = 0x80;
}

static void check_user_input(void) {
    if (mouse_dir != MOUSE_SPIN) {
	mouse_dir = 0;
	if (KEY_DOWN(X)) mouse_dir =  1;
	if (KEY_DOWN(Z)) mouse_dir = -1;
	if (MOUSE_HIT()) {
	    mouse_dir = MOUSE_SPIN;
	    punch();
	}
    }
    mouse_x += mouse_dir;
    if (mouse_x < MOUSE_START) {
	mouse_dir = -1;
	mouse_frame++;
	mouse_x = MOUSE_START;
    }
}

static void clear_screen(void) {
    SPRITE_ENABLE(0x0);
    memset(PTR(0xa000), COLOR_BLACK, 0x2000);
}

static void title_screen(void) {
    memset(PTR(0x8c00), COLOR(D_GREEN, L_GREEN), 1000);
    decompress(PTR(0xa000 +  LINE(32)), title);

    frame = 0;
    mouse_x = 140;
    mouse_y = 212;
    animate_mouse();

    // "Z" label
    set_sprite_pos(1, 159, 180);
    SPRITE_COLOR(1, COLOR_GREY);
    SPRITE_PTR(1, 0x2);

    // "X" label
    set_sprite_pos(2, 185, 180);
    SPRITE_COLOR(2, COLOR_GREY);
    SPRITE_PTR(2, 0x3);

    // left button
    set_sprite_pos(3, 159, 180);
    SPRITE_COLOR(3, COLOR_GREY);
    SPRITE_PTR(3, 0x0);

    // right button
    set_sprite_pos(4, 185, 180);
    SPRITE_COLOR(4, COLOR_GREY);
    SPRITE_PTR(4, 0x0);

    SPRITE_ENABLE(0x1f);

    do {
	char dir = frame & 0x40;
	wait_frame_finish();
	animate_mouse();
	button_switch(dir, !dir);
	mouse_dir = dir ? -1 : 1;
	mouse_x += mouse_dir;
	if (KEY_DOWN(RET)) return;
	frame++;
    } while (!KEY_DOWN(X) && !KEY_DOWN(Z));

    // arrow
    set_sprite_pos(5, 300, 180);
    SPRITE_COLOR(5, COLOR_L_GREEN);
    SPRITE_PTR(5, 0x4);

    do {
	wait_frame_finish();
	SPRITE_ENABLE((frame & 0x10) ? 0x3f : 0x1f);
	button_switch(KEY_DOWN(Z), KEY_DOWN(X));
	animate_mouse();
	check_user_input();
	frame++;
    } while (mouse_x < 320);
}

// array of structs would have been nicer but indexing array of structs
// involves multiplying index by size of struct and that is very very slow
unsigned short mob_x[8];
unsigned char  mob_y[8];
unsigned char  mob_frame[8];
signed char    mob_dy[8];
signed char    mob_dx[8];

static void (*level_increment)(void);

static void up_n_down(void) {
    static char i;
    for (i = 1; i < 8; i++) {
	if (mob_y[i] <= 120) mob_dy[i] =  1;
	if (mob_y[i] >= 180) mob_dy[i] = -1;
	mob_y[i] += mob_dy[i];
    }
}

static short row[] = { 0, 76, 108, 140, 172, 204, 236, 268 };

static void piston_pattern(unsigned char mask) {
    static char i;
    char dir = 1;

    for (i = 1; i < 8; i++) {
	mob_frame[i] = i & 1 ? 0 : 8;
	mob_dy[i] = dir;
	mob_x[i] = row[i];
	mob_y[i] = 150;
	dir = -dir;
    }

    level_increment = &up_n_down;
    SPRITE_ENABLE(mask);
}

static void piston_one(void) {
    piston_pattern(0x11);
}

static void piston_three(void) {
    piston_pattern(0x93);
}

static void piston_seven(void) {
    piston_pattern(0xff);
}

static char tent_h[] = { 0, 175, 150, 125, 125, 125, 150, 175 };

static void tent_six(void) {
    static char i;

    for (i = 1; i < 8; i++) {
	mob_frame[i] = i;
	mob_x[i] = row[i];
	mob_y[i] = tent_h[i];
	mob_dy[i] = (i & 4) ? -1 : 1;
    }

    level_increment = &up_n_down;
    SPRITE_ENABLE(0xef);
}

static short sin_row[] = { 0, 112, 132, 152, 172, 192, 212, 232 };

static void wave(void) {
    static char i;
    for (i = 1; i < 8; i++) {
	mob_y[i] = sin_h[(frame + mob_dy[i]) & 0x7f];
    }
}

static void sine_seven(void) {
    static char i;

    for (i = 1; i < 8; i++) {
	mob_frame[i] = i;
	mob_x[i] = sin_row[i];
	mob_dy[i] = -i * 16;
    }

    level_increment = &wave;
    level_increment();
    SPRITE_ENABLE(0xff);
}

static void pounce(void) {
    static char i;

    for (i = 1; i < 6; i++) {
	if (mob_y[i] >= 182) mob_dy[i] = -1;
	if (mob_y[i] <=  96) mob_dy[i] =  0;
	if (mob_dy[i] == 0 && (mob_x[i] - mouse_x) == 36) {
	    mob_dy[i] = 4;
	}
	mob_y[i] += mob_dy[i];
    }
}

static void pounce_five(void) {
    static char i;

    for (i = 1; i < 6; i++) {
	mob_frame[i] = i;
	mob_x[i] = row[i + 1];
	mob_y[i] = 96;
	mob_dy[i] = 0;
    }

    level_increment = &pounce;
    SPRITE_ENABLE(0x3f);
}

static void dance(void) {
    static char i, j;

    for (i = 1; i < 7; i++) {
	j = ((frame >> 0) + mob_dy[i]) & 0x7f;
	mob_x[i] = 150 + mob_dx[i] + dance_x[j];
	mob_y[i] = dance_y[j];
    }
}

static signed char dance_row[] = { 0, -60, -46, 10, 34, 90, 114, 0 };

static void dancing_six(void) {
    static char i;
    char offset = 0;

    for (i = 1; i < 7; i++) {
	mob_frame[i] = 0;
	mob_dx[i] = dance_row[i];
	mob_dy[i] = offset;
	offset = 96 - offset;
    }

    level_increment = &dance;
    level_increment();
    SPRITE_ENABLE(0x7f);
}

static void bounce(void) {
    static char i;

    for (i = 1; i < 5; i++) {
	if (mob_x[i] <= 56)  mob_dx[i] =  1;
	if (mob_x[i] >= 288) mob_dx[i] = -1;
	if (mob_y[i] <= 96)  mob_dy[i] =   1;
	if (mob_y[i] >= 186) mob_dy[i] =  -1;
	mob_x[i] += mob_dx[i];
	mob_y[i] += mob_dy[i];
    }
}

static void bounce_four(void) {
    mob_x[1] = 179;
    mob_y[1] = 167;
    mob_dx[1] =  1;
    mob_dy[1] = -1;

    mob_x[2] = 144;
    mob_y[2] = 147;
    mob_dx[2] = -1;
    mob_dy[2] = -1;

    mob_x[3] = 164;
    mob_y[3] = 112;
    mob_dx[3] = -1;
    mob_dy[3] =  1;

    mob_x[4] = 199;
    mob_y[4] = 132;
    mob_dx[4] =  1;
    mob_dy[4] =  1;

    level_increment = &bounce;
    SPRITE_ENABLE(0x1f);
}

static void oscilate(void) {
    static char i;
    for (i = 1; i < 8; i++) {
	if (mob_x[i] <= 56)  mob_dx[i] =  1;
	if (mob_x[i] >= 288) mob_dx[i] = -1;
	mob_y[i] = sin_h[(frame + mob_dy[i]) & 0x7f];
	mob_x[i] += mob_dx[i];
    }
}

static void oscilators(void) {
    static char i;

    for (i = 1; i < 8; i++) {
	mob_frame[i] = i;
	mob_x[i] = sin_row[i];
	mob_dx[i] = -1;
	mob_dy[i] = -i * 16;
    }

    level_increment = &oscilate;
    level_increment();
    SPRITE_ENABLE(0xff);
}

static void mouse_outro(void) {
    static char ptr = 0x11;
    if (mouse_x < 172) {
	mouse_x++;
    }
    else {
	if (note_delay == 0 && sid_freq_hi[music_index]) {
	    ptr = 0x23 - ptr;
	}
	SPRITE_PTR(0, ptr);
    }
}

static void end_game(void) {
    SPRITE_ENABLE(0x1);
    memset(PTR(0xd800), COLOR_BROWN, 800);
    memset(PTR(0xa000), COLOR_BLACK, 0x1500);
    memset(PTR(0x8c00), COLOR(ORANGE, YELLOW), 720);
    decompress(PTR(0xa000 +  LINE(48)), cheese);

    do {
	wait_frame_finish();
	animate_mouse();
	mouse_outro();
	play_music();
	frame++;
    } while (!KEY_DOWN(ESC));

    mouse_x = 400;
}

static void circle(void) {
    static char i;
    static char j;

    for (i = 1; i < 8; i++) {
	j = mob_dx[i] & 0x7f;
	mob_x[i] = sin_row[i] + circle_x[j];
	mob_y[i] = 152 + circle_y[j];
	mob_dx[i]--;
    }
}

static void circlers(void) {
    static char i;

    for (i = 1; i < 8; i++) {
	mob_dx[i] = i << 3;
	mob_frame[i] = i << 1;
    }

    level_increment = &circle;
    level_increment();
    SPRITE_ENABLE(0xff);
}

static unsigned char square_row[] = { 0, 92, 124, 156, 188, 220, 252, 0 };

static void square(void) {
    static char i;

    for (i = 1; i < 7; i++) {
	if (mob_dy[i] ==  32) {
	    mob_dx[i]++;
	    mob_x[i]++;
	}
	if (mob_dx[i] ==  32) {
	    mob_dy[i]--;
	    mob_y[i]--;
	}
	if (mob_dy[i] == -32) {
	    mob_dx[i]--;
	    mob_x[i]--;
	}
	if (mob_dx[i] == -32) {
	    mob_dy[i]++;
	    mob_y[i]++;
	}
    }
}

static void square_six(void) {
    static char i;

    for (i = 1; i < 7; i++) {
	mob_dx[i] = i & 1 ? -32 : 32;
	mob_frame[i] = i & 1 ? 4 : 12;
	mob_x[i] = square_row[i] + mob_dx[i];
	mob_y[i] = 150;
	mob_dy[i] = 0;
    }

    level_increment = &square;
    level_increment();
    SPRITE_ENABLE(0x7f);
}

static void hop_hop(void) {
    static char i;
    static char j;
    for (i = 1; i < 7; i++) {
	j = ((frame >> 0) + mob_dy[i]) & 0x7f;
	if (mob_x[i] <= 56)  mob_dx[i] =  1;
	if (mob_x[i] >= 288) mob_dx[i] = -1;
	mob_x[i] += mob_dx[i];
	mob_y[i] = dance_y[j];
    }
}

static void hoppers(void) {
    static char i;
    static char j;

    for (i = 1; i < 7; i++) {
	j = i < 4 ? -1 : 1;
	mob_frame[i] = i;
	mob_x[i] = sin_row[i];
	mob_dy[i] = j * (7 * i + 20);
	mob_dx[i] = j;
    }

    level_increment = &hop_hop;
    level_increment();
    SPRITE_ENABLE(0x7f);
}

unsigned char rot_row[] = { 0, 92, 92, 172, 172, 252, 252, 0 };

static void rotate(void) {
    static char i;
    static char j;

    for (i = 1; i < 7; i++) {
	j = mob_dx[i] & 0x7f;
	mob_x[i] = rot_row[i] + circle_x[j];
	mob_y[i] = 158 + circle_y[j];
	mob_dx[i]--;
    }
}

static void rotate_six(void) {
    static char i;

    for (i = 1; i < 7; i++) {
	mob_dx[i] = i & 1 ? 0 : 64;
	mob_frame[i] = i << 2;
    }

    level_increment = &rotate;
    level_increment();
    SPRITE_ENABLE(0x7f);
}

static void (*init[])(void) = {
    &piston_one,
    &piston_three,
    &tent_six,
    &sine_seven,
    &piston_seven,
    &pounce_five,
    &dancing_six,
    &bounce_four,
    &oscilators,
    &circlers,
    &square_six,
    &hoppers,
    &rotate_six,
    &end_game,
};

static unsigned char bat_ptrs[] = {
    0x18, 0x19, 0x1a, 0x1b,
    0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x1f, 0x1e, 0x1d,
    0x1c, 0x1b, 0x1a, 0x19,
};

static void animate_bats(void) {
    static char i;
    for (i = 1; i < 8; i++) {
	SPRITE_PTR(i, bat_ptrs[(frame + mob_frame[i]) & 0xf]);
	set_sprite_pos(i, mob_x[i], mob_y[i]);
    }
}

static void sid_cfg(void) {
    CFG_SID(0, 0x00, 0x08, 0x18, 0x1a, 0x11, 0x10);
    CFG_SID(2, 0x00, 0x08, 0x18, 0x1a, 0x11, 0x10);
}

static void advance_music(void) {
    note_delay = music_delay[music_index];

    music_index++;

    if (music_index == sizeof(music_delay)) {
	music_index = 0;
    }
}

static void attack0(void) {
    MEM(0xd400) = sid_freq_lo[music_index];
    MEM(0xd401) = sid_freq_hi[music_index];
    MEM(0xd404) = wave_form_on[0];
}

static void attack2(void) {
    MEM(0xd40e) = sid_freq_lo[music_index];
    MEM(0xd40f) = sid_freq_hi[music_index];
    MEM(0xd412) = wave_form_on[2];
}

static void relese0(void) {
    MEM(0xd404) = wave_form_off[0];
}

static void relese2(void) {
    MEM(0xd412) = wave_form_off[2];
}

static void play_music(void) {
    if (note_delay == 0) {
	if (sid_freq_hi[music_index]) {
	    voice_channel ? attack2() : attack0();
	}
	advance_music();
    }
    if (note_delay == note_release) {
	voice_channel ? relese2() : relese0();
	voice_channel = 2 - voice_channel;
    }
    note_delay--;
}

static void benchmark(void) {
    if (!(MEM(0xd011) & 0x80) && MEM(0xd012) > 150) {
	BORDER_COLOR(COLOR_D_RED);
    }
}

static void run_game(void) {
    char level = 1;
    mouse_x = MOUSE_START;
    mouse_y = 182;
    memset(PTR(0xd028), COLOR_L_GREEN, 7);
    memset(PTR(0x8c00), COLOR(L_RED, D_RED), 1000);
    decompress(PTR(0xa000 +  LINE(16)), tunnel);

    music_index = 0;
    animate_mouse();
    sid_cfg();
    init[0]();

    do {
	wait_frame_finish();
	animate_mouse();
	animate_bats();
	check_user_input();
	level_increment();
	play_music();
	frame++;
	if (mouse_x >= 320) {
	    if (level >= ARRAY_SIZE(init)) {
		break;
	    }
	    else {
		mouse_x = MOUSE_START;
		SPRITE_ENABLE(0x00);
		init[level]();
		level++;
	    }
	}
	else {
	    benchmark();
	}
    } while (!KEY_DOWN(ESC));
}

int main(void) {
    SWITCH_VIC_BANK();
    DISABLE_INTERRUPTS();
    MULTI_COLOR_BITMAP_MODE();
    BACKGROUND_COLOR(COLOR_BLACK);
    BORDER_COLOR(COLOR_BLACK);
    SPRITE_COMMON_COLORS_GREY();
    USE_SPRITE_DOUBLE_WIDTH();

    decompress(PTR(0x8000), button);
    decompress(PTR(0x8200), mouse);
    decompress(PTR(0x8400), spin);
    decompress(PTR(0x8600), bat);

    SPRITE_COLOR(0, COLOR_L_RED);
    memset(PTR(0xd800), COLOR_D_GREY, 1000);

    for (;;) {
	INIT_SID();
	clear_screen();
	title_screen();
	clear_screen();
	run_game();
    }
    return 0;
}
