
#pragma once

void mem_video_page(int which);
void mem_bank(int command);
void mem_map(int which);
void mem_write(unsigned int address, int value);
int mem_read(unsigned int address);
void mem_init();
