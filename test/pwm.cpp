#include <avr/io.h>
#include "notes.h"
#include "timer.h"
#include "pwm.h"

void setPWMFreq(int frequency)
{
    ICR1 = (frequency == 0) ? 0 : (F_CPU / (8 * frequency)) - 1;
}

struct note
{
    float frequency;
    int duration;
};

#define DTDHALF_NOTE 2433
#define HALF_NOTE 1622
#define QTR_NOTE 811
#define EIGTH_NOTE 405
#define SIXTEENTH_NOTE 203

struct note song[] = {
    {NOTE_REST, 1000},

    {NOTE_E4, SIXTEENTH_NOTE},
    {NOTE_G4, QTR_NOTE},
    {NOTE_B4, SIXTEENTH_NOTE},
    {NOTE_A4, SIXTEENTH_NOTE},
    {NOTE_G4, HALF_NOTE},

    {NOTE_E4, EIGTH_NOTE},
    {NOTE_G4, QTR_NOTE},
    {NOTE_E4, SIXTEENTH_NOTE},
    {NOTE_D4, SIXTEENTH_NOTE},
    {NOTE_C4, HALF_NOTE},
    {NOTE_REST, SIXTEENTH_NOTE},

    {NOTE_A4, EIGTH_NOTE},
    {NOTE_Fs4, EIGTH_NOTE},
    {NOTE_D4, EIGTH_NOTE},
    {NOTE_D4, EIGTH_NOTE},
    {NOTE_B3, EIGTH_NOTE},
    {NOTE_E4, QTR_NOTE},
    {NOTE_D4, EIGTH_NOTE},
    {NOTE_D4, EIGTH_NOTE + DTDHALF_NOTE},

    {NOTE_REST, 1000}};

enum playStates
{
    idle,
    playNote,
    waitNote,
    nextNote,
};

playStates playState = idle;
int currentNoteIndex = 0;
long noteStartTime = 0;
int songLength = (sizeof(song) / sizeof(song[0]));
long currentTimeTracker = 0;

void TickFct_PlaySong()
{

    switch (playState)
    {
    case idle:
        playState = playNote;
        noteStartTime = currentTimeTracker;

        break;

    case playNote:
        setPWMFreq(song[currentNoteIndex].frequency);
        playState = waitNote;
        break;

    case waitNote:
        if ((currentTimeTracker - noteStartTime) >= song[currentNoteIndex].duration)
        {
            playState = nextNote;
        }
        break;

    case nextNote:
        currentNoteIndex++;
        if (currentNoteIndex >= songLength)
        {
            currentNoteIndex = 0;
        }

        playState = playNote;
        noteStartTime = currentTimeTracker;

        break;

    default:
        break;
    }
}