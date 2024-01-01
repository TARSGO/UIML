
#pragma once

// 蜂鸣器命令数组（实际是字节序列）
typedef char BeepCmd[];

// MIDI音高（对应到beep.cpp表中的Hz）
// https://pages.mtu.edu/~suits/notefreqs.html
#define BEEP_C3     (char)48
#define BEEP_Cs3    (char)49
#define BEEP_D3     (char)50
#define BEEP_Ds3    (char)51
#define BEEP_E3     (char)52
#define BEEP_F3     (char)53
#define BEEP_Fs3    (char)54
#define BEEP_G3     (char)55
#define BEEP_Gs3    (char)56
#define BEEP_A3     (char)57
#define BEEP_As3    (char)58
#define BEEP_B3     (char)59
#define BEEP_C4     (char)60
#define BEEP_Cs4    (char)61
#define BEEP_D4     (char)62
#define BEEP_Ds4    (char)63
#define BEEP_E4     (char)64
#define BEEP_F4     (char)65
#define BEEP_Fs4    (char)66
#define BEEP_G4     (char)67
#define BEEP_Gs4    (char)68
#define BEEP_A4     (char)69
#define BEEP_As4    (char)70
#define BEEP_B4     (char)71
#define BEEP_C5     (char)72
#define BEEP_Cs5    (char)73
#define BEEP_D5     (char)74
#define BEEP_Ds5    (char)75
#define BEEP_E5     (char)76
#define BEEP_F5     (char)77
#define BEEP_Fs5    (char)78
#define BEEP_G5     (char)79
#define BEEP_Gs5    (char)80
#define BEEP_A5     (char)81
#define BEEP_As5    (char)82
#define BEEP_B5     (char)83
#define BEEP_C6     (char)84
#define BEEP_Cs6    (char)85
#define BEEP_D6     (char)86
#define BEEP_Ds6    (char)87
#define BEEP_E6     (char)88
#define BEEP_F6     (char)89
#define BEEP_Fs6    (char)90
#define BEEP_G6     (char)91
#define BEEP_Gs6    (char)92
#define BEEP_A6     (char)93
#define BEEP_As6    (char)94
#define BEEP_B6     (char)95
#define BEEP_C7     (char)96
#define BEEP_Cs7    (char)97
#define BEEP_D7     (char)98
#define BEEP_Ds7    (char)99
#define BEEP_E7     (char)100
#define BEEP_F7     (char)101
#define BEEP_Fs7    (char)102
#define BEEP_G7     (char)103
#define BEEP_Gs7    (char)104
#define BEEP_A7     (char)105
#define BEEP_As7    (char)106
#define BEEP_B7     (char)107

#define BEEP_LOWEST_PITCH BEEP_C3
#define BEEP_HIGHEST_PITCH BEEP_B7
