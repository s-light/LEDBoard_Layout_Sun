/******************************************************************************

    effect engine

******************************************************************************/
/******************************************************************************
    The MIT License (MIT)

    Copyright (c) 2018 Stefan Krüger

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
******************************************************************************/

#include "./effect_engine.h"

namespace effect_engine {
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// definitions
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

sequencer_modes sequencer_mode = sequencer_OFF;

uint16_t value_low = 1;
uint16_t value_high = 1000;

uint32_t sequencer_timestamp_last = millis();
uint32_t sequencer_interval = 1000;

int16_t sequencer_current_step = 0;
uint8_t sequencer_direction_forward = true;

uint16_t sequencer_color[LEDBoard::colors_per_led] = {     0, 10000,     0};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// private function definitions
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// if they are in the correct order you don't need to define theme :-)

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// private functions
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



// parse the pointer to the first element of the effect_map
void calculate_step__effectmap(
    const uint8_t *effect_map,
    const uint8_t row_count,
    const uint8_t column_count,
    const uint8_t board_start_index,
    // const uint16_t tail[][LEDBoard::colors_per_led],
    const uint16_t tail[][3],
    const uint8_t tail_count
) {
    // Serial.println("calculate_step__effectmap: ");

    uint16_t ch_offset = LEDBoard::colorchannels_per_board * board_start_index;

    uint8_t boards_per_row = column_count / LEDBoard::leds_per_row;
    uint8_t boards_per_column = row_count / LEDBoard::leds_per_column;

    for (size_t row = 0; row < row_count; row++) {
        for (size_t column = 0; column < column_count; column++) {
            // Serial.print("step ");
            // Serial.print(sequencer_current_step);
            // Serial.print("; r");
            // Serial.print(row);
            // Serial.print("; c");
            // Serial.print(column);

            uint8_t pixel = 0;
            uint16_t ch = 0;

            uint8_t board_column = column;
            uint8_t board_row = row;

            if (column >= LEDBoard::leds_per_row) {
                board_column = column % LEDBoard::leds_per_row;
            }
            if (row >= LEDBoard::leds_per_column) {
                board_row = row % LEDBoard::leds_per_column;
            }

            // Serial.print("; br: ");
            // Serial.print(board_row);
            // Serial.print("; bc: ");
            // Serial.print(board_column);

            // calculate board offset count

            uint8_t boards_offset_row = (row / LEDBoard::leds_per_column);
            uint8_t boards_offset_column = (column / LEDBoard::leds_per_row);
            uint8_t boards_offset =
                (boards_offset_row * boards_per_row) + boards_offset_column;
            // Serial.print("; bor: ");
            // Serial.print(boards_offset_row);
            // Serial.print("; boc: ");
            // Serial.print(boards_offset_column);
            // Serial.print("; bo: ");
            // Serial.print(boards_offset);

            pixel = LEDBoard::channel_position_map[board_row][board_column];
            ch = pixel * 3;
            // Serial.print("; ch: ");
            // Serial.print(ch);

            ch = ch +
                (LEDBoard::colorchannels_per_board * (uint16_t)boards_offset);
            // Serial.print("; ch: ");
            // Serial.print(ch);

            // Serial.print("; ch_offset: ");
            // Serial.print(ch_offset + );

            // Serial.print("; pixel: ");
            // Serial.print(pixel);
            // Serial.print("; ch: ");
            // Serial.print(ch);


            // uint8_t effect_step = effect_map[row][column];
            uint8_t effect_step = effect_map[(row *column_count) + column];

            // if (effect_step == sequencer_current_step) {
            //     // Serial.print(" ON");
            //     // set pixel to high
            //     values[ch + 0] = 20000;
            //     values[ch + 1] = 55000;
            //     values[ch + 2] = 0;
            //     // values[ch + 0] = 1000;
            //     // values[ch + 1] = 4000;
            //     // values[ch + 2] = 0;
            // }
            // else {
            //     // set pixel to low
            //     values[ch + 0] = 0;
            //     values[ch + 1] = 0;
            //     values[ch + 2] = 0;
            // }

            // tail
            int8_t tail_step = effect_step - sequencer_current_step;

            if (sequencer_direction_forward) {
                // change tail direction
                tail_step = tail_count - tail_step;
            }

            // add offset
            ch = ch_offset + ch;

            if ((tail_step >= 0) && (tail_step < tail_count)) {
                LEDBoard::tlc.setChannel(ch + 0, tail[tail_step][0]);
                LEDBoard::tlc.setChannel(ch + 1, tail[tail_step][1]);
                LEDBoard::tlc.setChannel(ch + 2, tail[tail_step][2]);
            }
            // else {
            //     // set pixel to low
            //     LEDBoard::tlc.setChannel(ch + 0, 0);
            //     LEDBoard::tlc.setChannel(ch + 1, 0);
            //     LEDBoard::tlc.setChannel(ch + 2, 0);
            // }

            // Serial.println();
        }
    }
}



void map_to_allBoards() {
    if (LEDBoard::output_enabled) {
        // set all channels (mapping)
        for (
            size_t channel_index = 0;
            channel_index < LEDBoard::colorchannels_per_board;
            channel_index++
        ) {
            // uint8_t mapped_channel = mapping_single_board[i];
            // Serial.print("mapping: ");
            // Serial.print(i);
            // Serial.print("-->");
            // Serial.print(mapped_channel);
            // Serial.println();
            for (
                size_t board_index = 0;
                board_index < LEDBoard::boards_count;
                board_index++
            ) {
                LEDBoard::tlc.setChannel(
                    channel_index +
                    (LEDBoard::tlc_channels_per_board * board_index),
                    LEDBoard::tlc.getChannel(channel_index));
            }
        }
    }
}

void map_to_nBoards(
  uint8_t board_start_index,
  uint8_t boards_count_local,
  uint8_t boards_per_copy
) {
    if (LEDBoard::output_enabled) {
        // set all channels (mapping)
        for (
            size_t board_index = board_start_index;
            board_index < (board_start_index + boards_count_local);
            board_index += boards_per_copy
        ) {
            // Serial.print("bi: ");
            // Serial.print(board_index);
            // Serial.println();
            // copy channels for both boards
            for (
                size_t channel_index = 0;
                channel_index <
                    (LEDBoard::colorchannels_per_board*boards_per_copy);
                channel_index++
            ) {
                // uint8_t mapped_channel = mapping_single_board[i];
                // Serial.print("mapping: ");
                // Serial.print(i);
                // Serial.print("-->");
                // Serial.print(mapped_channel);
                // Serial.println();

                // get channel value
                uint16_t temp_value = LEDBoard::tlc.getChannel(
                  channel_index +
                  (LEDBoard::tlc_channels_per_board * (board_start_index)));
                // set channel value
                LEDBoard::tlc.setChannel(
                    channel_index +
                    (LEDBoard::tlc_channels_per_board * (board_index)),
                    temp_value);
            }
        }
    }
}



// void set_hsv_color(uint16_t hue, uint16_t saturation, uint16_t value) {
//     map(hue, 0,)
// }
void set_hsv_color(uint8_t hue, uint8_t saturation, uint8_t value) {
    CRGB rgb8bit;
    hsv2rgb_rainbow(CHSV(hue, saturation, value), rgb8bit);
    // uint16_t(rgb8bit.r) << 8
    // is good enough (we don't reach 100% this way...)
    sequencer_color[0] = uint16_t(rgb8bit.r) << 8;
    sequencer_color[1] = uint16_t(rgb8bit.g) << 8;
    sequencer_color[2] = uint16_t(rgb8bit.b) << 8;
    // sequencer_color[0] = map(rgb8bit.r, 0, 255, 0, 65535);
    // sequencer_color[1] = map(rgb8bit.g, 0, 255, 0, 65535);
    // sequencer_color[2] = map(rgb8bit.b, 0, 255, 0, 65535);
}



void sequencer_off() {
    Serial.println("sequencer_off");
    // uint16_t values[LEDBoard::colorchannels_per_board];
    // // init array with 0
    // memset(values, 0, LEDBoard::colorchannels_per_board);
    //
    // for (size_t ch = 0; ch < LEDBoard::colorchannels_per_board; ch++) {
    //     values[ch] = value_low;
    // }
    LEDBoard::tlc.setRGB(0, 0, value_low);

    // reset sequencer
    sequencer_current_step = 0;

    // now map values to LEDBoard::tlc chips and write theme to output
    // map_to_allBoards(values);
}


void calculate_step__channelcheck() {
    // Serial.print("calculate_step__channelcheck: ");

    for (size_t ch = 0; ch < LEDBoard::colorchannels_per_board; ch++) {
        if (ch == (uint8_t)sequencer_current_step) {
            // set pixel to high
            LEDBoard::tlc.setChannel(ch, value_high);
        } else {
            // set pixel to low
            LEDBoard::tlc.setChannel(ch, value_low);
        }
    }
}

void calculate_step__channelcheck_next() {
    // prepare next step
    // Serial.print("sequencer_current_step: ");
    // Serial.println(sequencer_current_step);
    sequencer_current_step = sequencer_current_step + 1;
    if (sequencer_current_step >= LEDBoard::colorchannels_per_board) {
        sequencer_current_step = 0;
    }
}


void calculate_step__spiral() {
    // Serial.println("calculate_step__spiral: ");

    const uint8_t spiral_order
        [LEDBoard::leds_per_column][LEDBoard::leds_per_row] {
        { 0,  1,  2,  3},
        {11, 12, 13,  4},
        {10, 15, 14,  5},
        { 9,  8,  7,  6},
    };

    for (size_t column = 0; column < LEDBoard::leds_per_column; column++) {
        for (size_t row = 0; row < LEDBoard::leds_per_row; row++) {
            uint8_t pixel = LEDBoard::channel_position_map[column][row];
            uint8_t ch = pixel * 3;

            // set pixel to low
            // values[ch + 0] = value_low;
            // values[ch + 1] = value_low;
            // values[ch + 2] = value_low;

            if (spiral_order[column][row] == (uint8_t)sequencer_current_step) {
                // set pixel to high
                LEDBoard::tlc.setChannel(ch + 0, sequencer_color[0]);
                LEDBoard::tlc.setChannel(ch + 1, sequencer_color[1]);
                LEDBoard::tlc.setChannel(ch + 2, sequencer_color[2]);
            } else {
                // set pixel to low
                LEDBoard::tlc.setChannel(ch + 0, 0);
                LEDBoard::tlc.setChannel(ch + 1, 0);
                LEDBoard::tlc.setChannel(ch + 2, 0);
            }
        }
    }
}

void calculate_step__spiral_next() {
    // prepare next step
    // Serial.print("sequencer_current_step: ");
    // Serial.println(sequencer_current_step);
    if (sequencer_direction_forward) {
        // forward
        if (
            sequencer_current_step >=
            ((LEDBoard::leds_per_column * LEDBoard::leds_per_row) - 1)
        ) {
            sequencer_current_step = sequencer_current_step - 1;
            sequencer_direction_forward = false;
            // Serial.println("direction switch to backwards");
        } else {
            sequencer_current_step = sequencer_current_step + 1;
        }
    } else {
        // backwards
        if (sequencer_current_step == 0) {
            sequencer_current_step = sequencer_current_step + 1;
            sequencer_direction_forward = true;
            // Serial.println("direction switch to forward");
        } else {
            sequencer_current_step = sequencer_current_step - 1;
        }
    }
    // Serial.print("next step: ");
    // Serial.println(sequencer_current_step);
}


void calculate_step__horizontal() {
    // Serial.println("calculate_step__horizontal: ");

    for (size_t column = 0; column < LEDBoard::leds_per_row; column++) {
        for (size_t row = 0; row < LEDBoard::leds_per_column; row++) {
            uint8_t pixel = LEDBoard::channel_position_map[column][row];
            uint8_t ch = pixel * 3;

            // tail
            int8_t tail_step = column - sequencer_current_step;
            tail_step = tail_water_count - tail_step;
            // if (!sequencer_direction_forward) {
            //     // change tail direction
            //     tail_step = tail_count - tail_step;
            // }
            //
            // add offset
            // ch = ch_offset + ch;

            if ((tail_step >= 0) && (tail_step < tail_water_count)) {
                LEDBoard::tlc.setChannel(ch + 0, tail_water[tail_step][0]);
                LEDBoard::tlc.setChannel(ch + 1, tail_water[tail_step][1]);
                LEDBoard::tlc.setChannel(ch + 2, tail_water[tail_step][2]);
            } else {
                // set pixel to low
                LEDBoard::tlc.setChannel(ch + 0, 0);
                LEDBoard::tlc.setChannel(ch + 1, 0);
                LEDBoard::tlc.setChannel(ch + 2, 0);
            }
        }
    }
}

void calculate_step__horizontal_next() {
    // prepare next step
    // Serial.print("sequencer_current_step: ");
    // Serial.println(sequencer_current_step);
    // sequencer_current_step = sequencer_current_step + 1;
    // if (sequencer_current_step >= LEDBoard::leds_per_column) {
    //     sequencer_current_step = 0;
    // }
    if (sequencer_current_step > 0) {
        sequencer_current_step = sequencer_current_step - 1;
    } else {
        sequencer_current_step = LEDBoard::leds_per_column-1;
    }
    // if (sequencer_direction_forward) {
    //     // forward
    //     if (sequencer_current_step >= LEDBoard::leds_per_column-1 ) {
    //         sequencer_current_step = sequencer_current_step - 1;
    //         sequencer_direction_forward = false;
    //         // Serial.println("direction switch to backwards");
    //     }
    //     else {
    //         sequencer_current_step = sequencer_current_step + 1;
    //     }
    // }
    // else {
    //     // backwards
    //     if (sequencer_current_step == 0) {
    //         sequencer_current_step = sequencer_current_step + 1;
    //         sequencer_direction_forward = true;
    //         // Serial.println("direction switch to forward");
    //     }
    //     else {
    //         sequencer_current_step = sequencer_current_step - 1;
    //     }
    // }
}


void calculate_step__hpline() {
    // Serial.println("calculate_step__spiral: ");

    for (size_t column = 0; column < LEDBoard::leds_per_column; column++) {
        uint8_t pixel = LEDBoard::channel_position_map[0][column];
        uint8_t ch = pixel * 3;

        if (column == (uint8_t)sequencer_current_step) {
            // set pixel to high
            LEDBoard::tlc.setChannel(ch + 0, 0);
            LEDBoard::tlc.setChannel(ch + 1, 65535);
            LEDBoard::tlc.setChannel(ch + 2, 65535);
        } else {
            // set pixel to low
            LEDBoard::tlc.setChannel(ch + 0, 0);
            LEDBoard::tlc.setChannel(ch + 1, 40000);
            LEDBoard::tlc.setChannel(ch + 2, 65535);
        }
    }
}

void calculate_step__hpline_next() {
    // prepare next step
    // Serial.print("sequencer_current_step: ");
    // Serial.println(sequencer_current_step);
    // sequencer_current_step = sequencer_current_step + 1;
    // if (sequencer_current_step >= LEDBoard::leds_per_column) {
    //     sequencer_current_step = 0;
    // }
    if (sequencer_current_step == 0) {
        sequencer_current_step = LEDBoard::leds_per_column-1;
    } else {
        sequencer_current_step = sequencer_current_step - 1;
    }

    // reset step numbers to range..
    if (sequencer_current_step >= LEDBoard::leds_per_column) {
        sequencer_current_step = LEDBoard::leds_per_column-1;
    }
}



void calculate_step__spiral2(
    const uint8_t board_start_index = 0,
    const bool flag_horizontal = false
) {
    const uint8_t row_count_vertical = LEDBoard::leds_per_column*2;
    const uint8_t column_count_vertical = LEDBoard::leds_per_row;
    const uint8_t spiral_order_vertical
        [row_count_vertical][column_count_vertical] {
        { 7,  8,  9, 10},
        { 6, 25, 26, 11},
        { 5, 24, 27, 12},
        { 4, 23, 28, 13},

        { 3, 22, 29, 14},
        { 2, 21, 30, 15},
        { 1, 20, 31, 16},
        { 0, 19, 18, 17},
    };

    const uint8_t row_count_horizontal = LEDBoard::leds_per_column;
    const uint8_t column_count_horizontal = LEDBoard::leds_per_row*2;
    const uint8_t spiral_order_horizontal
        [row_count_horizontal][column_count_horizontal] {
        {17, 16, 15, 14,   13, 12, 11, 10},
        {18, 31, 30, 29,   28, 27, 26,  9},
        {19, 20, 21, 22,   23, 24, 25,  8},
        { 0,  1,  2,  3,    4,  5,  6,  7},
    };

    if (flag_horizontal) {
        calculate_step__effectmap(
            &spiral_order_horizontal[0][0],
            row_count_horizontal,
            column_count_horizontal,
            board_start_index,
            tail_orange,
            tail_orange_count);
    } else {
        calculate_step__effectmap(
            &spiral_order_vertical[0][0],
            row_count_vertical,
            column_count_vertical,
            board_start_index,
            tail_orange,
            tail_orange_count);
    }
}

void calculate_step__sun_spiral_center3(const uint8_t board_start_index = 0) {
    // Serial.println("calculate_step__sun_spiral_center3: ");

    const uint8_t column_count = LEDBoard::leds_per_row*3;
    const uint8_t row_count = LEDBoard::leds_per_column*1;
    const uint8_t spiral_order[row_count][column_count] {
        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11},
        {13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 12},
        {12, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13},
        {11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0},
    };

    calculate_step__effectmap(
        &spiral_order[0][0],
        row_count,
        column_count,
        board_start_index,
        tail_orange,
        tail_orange_count);
}

void calculate_step__spiral2_next() {
    // Serial.println("calculate_step__spiral: ");

    // prepare next step
    // Serial.print("sequencer_current_step: ");
    // Serial.println(sequencer_current_step);
    const uint8_t effect_step_count = (LEDBoard::leds_per_board * 2);
    if (sequencer_direction_forward) {
        // forward
        if (sequencer_current_step > effect_step_count) {
            sequencer_current_step = sequencer_current_step - 1;
            sequencer_direction_forward = false;
            // Serial.println("direction switch to backwards");
        } else {
            sequencer_current_step = sequencer_current_step + 1;
        }
    } else {
        // backwards
        if (sequencer_current_step <= (tail_orange_count*-1)) {
            sequencer_current_step = sequencer_current_step + 1;
            sequencer_direction_forward = true;
            // Serial.println("direction switch to forward");
        } else {
            sequencer_current_step = sequencer_current_step - 1;
        }
    }
    // Serial.print("next step: ");
    // Serial.println(sequencer_current_step);
}


void calculate_step__wave4(
    uint8_t board_start_index = 0,
    bool flag_horizontal = false,
    const uint16_t tail[][3] = tail_water,
    const uint8_t tail_count = tail_water_count
) {
    const uint8_t row_count_vertical = LEDBoard::leds_per_column*4;
    const uint8_t column_count_vertical = LEDBoard::leds_per_row;
    const uint8_t effect_order_vertical
        [row_count_vertical][column_count_vertical] {
        { 0,  0,  0,  0},
        { 1,  1,  1,  1},
        { 2,  2,  2,  2},
        { 3,  3,  3,  3},

        { 4,  4,  4,  4},
        { 5,  5,  5,  5},
        { 6,  6,  6,  6},
        { 7,  7,  7,  7},

        { 8,  8,  8,  8},
        { 9,  9,  9,  9},
        {10, 10, 10, 10},
        {11, 11, 11, 11},

        {12, 12, 12, 12},
        {13, 13, 13, 13},
        {14, 14, 14, 14},
        {15, 15, 15, 15},
    };

    const uint8_t row_count_horizontal = LEDBoard::leds_per_column;
    const uint8_t column_count_horizontal = LEDBoard::leds_per_row*4;
    const uint8_t effect_order_horizontal
        [row_count_horizontal][column_count_horizontal] {
        { 0,  0,  0,  0,    0,  0,  0,  0,    0,  0,  0,  0,    0,  0,  0,  0},
        { 1,  1,  1,  1,    1,  1,  1,  1,    1,  1,  1,  1,    1,  1,  1,  1},
        { 2,  2,  2,  2,    2,  2,  2,  2,    2,  2,  2,  2,    2,  2,  2,  2},
        { 3,  3,  3,  3,    3,  3,  3,  3,    3,  3,  3,  3,    3,  3,  3,  3},
    };

    if (flag_horizontal) {
        calculate_step__effectmap(
            &effect_order_horizontal[0][0],
            row_count_horizontal,
            column_count_horizontal,
            board_start_index,
            tail,
            tail_count);
    } else {
        calculate_step__effectmap(
            &effect_order_vertical[0][0],
            row_count_vertical,
            column_count_vertical,
            board_start_index,
            tail,
            tail_count);
    }
}

void calculate_step__wave4_next(uint8_t tail_count = tail_water_count) {
    // prepare next step
    // Serial.print("sequencer_current_step: ");
    // Serial.println(sequencer_current_step);
    const uint8_t effect_step_count = (16);
    // without direction change
    // 'forward'
    if (sequencer_current_step < effect_step_count) {
        sequencer_current_step = sequencer_current_step + 1;
    } else {
        sequencer_current_step = (tail_count*-1)+1;
        // Serial.println("line4_next: start new itteration");
    }
    // backwards
    // sequencer_direction_forward = false;
    // if (sequencer_current_step <= (tail_count*-1)) {
    //     sequencer_current_step = effect_step_count;
    //     Serial.println("line4_next: start new itteration");
    // }
    // else {
    //     sequencer_current_step = sequencer_current_step - 1;
    // }

    // with direction change
    // if (sequencer_direction_forward) {
    //     // forward
    //     if (sequencer_current_step > effect_step_count) {
    //         sequencer_current_step = sequencer_current_step - 1;
    //         sequencer_direction_forward = false;
    //         // Serial.println("direction switch to backwards");
    //     }
    //     else {
    //         sequencer_current_step = sequencer_current_step + 1;
    //     }
    // }
    // else {
    //     // backwards
    //     if (sequencer_current_step <= (tail_count*-1)) {
    //         sequencer_current_step = sequencer_current_step + 1;
    //         sequencer_direction_forward = true;
    //         // Serial.println("direction switch to forward");
    //     }
    //     else {
    //         sequencer_current_step = sequencer_current_step - 1;
    //     }
    // }

    // Serial.print("next step: ");
    // Serial.println(sequencer_current_step);
}


void calculate_step__wave_center2x4(
    const uint8_t board_start_index = 0,
    const uint16_t tail[][3] = tail_water,
    const uint8_t tail_count = tail_water_count
) {
    // const uint8_t row_count = LEDBoard::leds_per_column;
    // const uint8_t column_count = LEDBoard::leds_per_row*8;
    // const uint8_t effect_order[row_count][column_count] {
    //     { 0,  0,  0,  0,    0,  0,  0,  0,    0,  0,  0,  0,    0,  0,  0,  0,    3,  3,  3,  3,    3,  3,  3,  3,    3,  3,  3,  3,    3,  3,  3,  3},
    //     { 1,  1,  1,  1,    1,  1,  1,  1,    1,  1,  1,  1,    1,  1,  1,  1,    2,  2,  2,  2,    2,  2,  2,  2,    2,  2,  2,  2,    2,  2,  2,  2},
    //     { 2,  2,  2,  2,    2,  2,  2,  2,    2,  2,  2,  2,    2,  2,  2,  2,    1,  1,  1,  1,    1,  1,  1,  1,    1,  1,  1,  1,    1,  1,  1,  1},
    //     { 3,  3,  3,  3,    3,  3,  3,  3,    3,  3,  3,  3,    3,  3,  3,  3,    0,  0,  0,  0,    0,  0,  0,  0,    0,  0,  0,  0,    0,  0,  0,  0},
    // };
    const uint8_t row_count = LEDBoard::leds_per_column*2;
    const uint8_t column_count = LEDBoard::leds_per_row*4;
    const uint8_t effect_order[row_count][column_count] {
        { 0,  0,  0,  0,    0,  0,  0,  0,    0,  0,  0,  0,    0,  0,  0,  0},
        { 1,  1,  1,  1,    1,  1,  1,  1,    1,  1,  1,  1,    1,  1,  1,  1},
        { 2,  2,  2,  2,    2,  2,  2,  2,    2,  2,  2,  2,    2,  2,  2,  2},
        { 3,  3,  3,  3,    3,  3,  3,  3,    3,  3,  3,  3,    3,  3,  3,  3},
        { 3,  3,  3,  3,    3,  3,  3,  3,    3,  3,  3,  3,    3,  3,  3,  3},
        { 2,  2,  2,  2,    2,  2,  2,  2,    2,  2,  2,  2,    2,  2,  2,  2},
        { 1,  1,  1,  1,    1,  1,  1,  1,    1,  1,  1,  1,    1,  1,  1,  1},
        { 0,  0,  0,  0,    0,  0,  0,  0,    0,  0,  0,  0,    0,  0,  0,  0},
    };

    calculate_step__effectmap(
        &effect_order[0][0],
        row_count,
        column_count,
        board_start_index,
        tail,
        tail_count);
}

void calculate_step__wave_center3(
    const uint8_t board_start_index = 0,
    const bool flag_mirror = false,
    const uint16_t tail[][3] = tail_water,
    const uint8_t tail_count = tail_water_count
) {
    const uint8_t row_count_horizontal = LEDBoard::leds_per_column;
    const uint8_t column_count_horizontal = LEDBoard::leds_per_row*3;
    const uint8_t effect_order_horizontal_mirror
        [row_count_horizontal][column_count_horizontal] {
        {7,  7,    7,  7,  7,  7,    7,  7,  7,  7,    7,  7},
        {6,  6,    6,  6,  6,  6,    6,  6,  6,  6,    6,  6},
        {5,  5,    5,  5,  5,  5,    5,  5,  5,  5,    5,  5},
        {4,  4,    4,  4,  4,  4,    4,  4,  4,  4,    4,  4},
    };
    const uint8_t effect_order_horizontal
        [row_count_horizontal][column_count_horizontal] {
        {4,  4,    4,  4,  4,  4,    4,  4,  4,  4,    4,  4},
        {5,  5,    5,  5,  5,  5,    5,  5,  5,  5,    5,  5},
        {6,  6,    6,  6,  6,  6,    6,  6,  6,  6,    6,  6},
        {7,  7,    7,  7,  7,  7,    7,  7,  7,  7,    7,  7},
    };

    if (flag_mirror) {
        calculate_step__effectmap(
            &effect_order_horizontal_mirror[0][0],
            row_count_horizontal,
            column_count_horizontal,
            board_start_index,
            tail,
            tail_count);
    } else {
        calculate_step__effectmap(
            &effect_order_horizontal[0][0],
            row_count_horizontal,
            column_count_horizontal,
            board_start_index,
            tail,
            tail_count);
    }
}


void calculate_step_mounting_sun_spiral() {
    // Serial.print("calculate_step: ");

    uint8_t board_start_index = 0;

    // create animaiton in center
    // first row (3 boards)
    calculate_step__sun_spiral_center3(board_start_index);
    board_start_index += 3;


    // // second row (4 boards)
    calculate_step__spiral2(board_start_index, true);
    board_start_index += 2;
    calculate_step__spiral2(board_start_index, true);
    board_start_index += 2;

    // thirt row (4 boards)
    calculate_step__spiral2(board_start_index, true);
    board_start_index += 2;
    calculate_step__spiral2(board_start_index, true);
    board_start_index += 2;

    // fourth row (3 boards)
    calculate_step__sun_spiral_center3(board_start_index);
    board_start_index += 3;


    // use spiral2 for arms
    calculate_step__spiral2(board_start_index);
    // copy to all arms
    map_to_nBoards(board_start_index, LEDBoard::boards_count_sun_arms, 2);
}

void calculate_step_mounting_sun_waves_blue() {
    // Serial.print("calculate_step: ");

    uint8_t board_start_index = 0;

    // create animaiton in center
    // first row (3 boards)
    calculate_step__wave_center3(board_start_index, false);
    board_start_index += 3;

    // second row (4 boards)
    // thirt row (4 boards)
    calculate_step__wave_center2x4(board_start_index);
    board_start_index += 4;
    board_start_index += 4;

    // fourth row (3 boards)
    calculate_step__wave_center3(board_start_index, true);
    board_start_index += 3;

    // use spiral2 for arms
    calculate_step__wave4(board_start_index);
    // copy to all arms
    map_to_nBoards(board_start_index, LEDBoard::boards_count_sun_arms, 4);
}

void calculate_step_mounting_sun_waves_orange() {
    // Serial.print("calculate_step: ");

    uint8_t board_start_index = 0;

    // create animaiton in center
    // first row (3 boards)
    calculate_step__wave_center3(
        board_start_index,
        false,
        tail_sun,
        tail_sun_count);
    board_start_index += 3;

    // second row (4 boards)
    // thirt row (4 boards)
    calculate_step__wave_center2x4(
        board_start_index,
        tail_sun,
        tail_sun_count);
    board_start_index += 4;
    board_start_index += 4;

    // fourth row (3 boards)
    calculate_step__wave_center3(
        board_start_index,
        true,
        tail_sun,
        tail_sun_count);
    board_start_index += 3;

    // use spiral2 for arms
    calculate_step__wave4(
        board_start_index,
        false,
        tail_sun,
        tail_sun_count);
    // copy to all arms
    map_to_nBoards(board_start_index, LEDBoard::boards_count_sun_arms, 4);
}



void calculate_step_singleboard() {
    // Serial.print("calculate_step: ");

    // we use the a part of our global memory.
    // uint16_t values_dualboard[LEDBoard::colorchannels_per_board];
    // init array with 0
    // memset(, 0, LEDBoard::colorchannels_per_board);

    // deside what sequencer we want to run

    switch (sequencer_mode) {
        case sequencer_OFF:
        case sequencer_HORIZONTAL4:
        case sequencer_SPIRAL2:
        case sequencer_SUN_SPIRAL:
        case sequencer_SUN_WAVE_BLUE:
        case sequencer_SUN_WAVE_ORANGE: {
            // 1;
        } break;
        case sequencer_CHANNELCHECK: {
            calculate_step__channelcheck();
        } break;
        case sequencer_HORIZONTAL: {
            calculate_step__horizontal();
        } break;
        case sequencer_SPIRAL: {
            calculate_step__spiral();
        } break;
        case sequencer_HPLINE: {
            calculate_step__hpline();
        } break;
    }

    // debug out print array:
    // Serial.print("values_global: ");
    // slight_DebugMenu::print_uint16_array(
    //     Serial,
    //     values_global,
    //     LEDBoard::colorchannels_per_board
    // );
    // Serial.println();

    // now map to all LEDBoard::tlc chips and write theme to output
    map_to_allBoards();
}

void calculate_step_singleboard_next() {
    switch (sequencer_mode) {
        case sequencer_OFF:
        case sequencer_HORIZONTAL4:
        case sequencer_SPIRAL2:
        case sequencer_SUN_SPIRAL:
        case sequencer_SUN_WAVE_BLUE:
        case sequencer_SUN_WAVE_ORANGE: {
            // 1;
        } break;
        case sequencer_CHANNELCHECK: {
            calculate_step__channelcheck_next();
        } break;
        case sequencer_HORIZONTAL: {
            calculate_step__horizontal_next();
        } break;
        case sequencer_SPIRAL: {
            calculate_step__spiral_next();
        } break;
        case sequencer_HPLINE: {
            calculate_step__hpline_next();
        } break;
    }
}


void calculate_step() {
    // Serial.println("calculate_step ");
    switch (sequencer_mode) {
        case sequencer_OFF: {
            // 1;
        } break;
        case sequencer_CHANNELCHECK:
        case sequencer_HORIZONTAL:
        case sequencer_SPIRAL:
        case sequencer_HPLINE: {
            calculate_step_singleboard();
        } break;
        case sequencer_SPIRAL2: {
            calculate_step__spiral2();
            // now map to all LEDBoard::tlc chips and write theme to output
            map_to_nBoards(0, LEDBoard::boards_count, 2);
        } break;
        case sequencer_HORIZONTAL4: {
            calculate_step__wave4();
            // now map to all LEDBoard::tlc chips and write theme to output
            map_to_nBoards(0, LEDBoard::boards_count, 2);
        } break;
        case sequencer_SUN_SPIRAL: {
            calculate_step_mounting_sun_spiral();
        } break;
        case sequencer_SUN_WAVE_BLUE: {
            calculate_step_mounting_sun_waves_blue();
        } break;
        case sequencer_SUN_WAVE_ORANGE: {
            calculate_step_mounting_sun_waves_orange();
        } break;
    }
    if (sequencer_mode > sequencer_OFF) {
        // write data to chips
        LEDBoard::tlc.write();
    }
}

void calculate_step_next() {
    // Serial.println("calculate_step ");
    switch (sequencer_mode) {
        case sequencer_OFF: {
            // 1;
        } break;
        case sequencer_CHANNELCHECK:
        case sequencer_HORIZONTAL:
        case sequencer_SPIRAL:
        case sequencer_HPLINE: {
            calculate_step_singleboard_next();
        } break;
        case sequencer_SPIRAL2: {
            calculate_step__spiral2_next();
        } break;
        case sequencer_HORIZONTAL4: {
            calculate_step__wave4_next();
        } break;
        case sequencer_SUN_SPIRAL: {
            // calculate_step_mounting_sun_spiral_next();
            calculate_step__spiral2_next();
        } break;
        case sequencer_SUN_WAVE_BLUE: {
            // calculate_step_mounting_sun_waves_blue_next();
            calculate_step__wave4_next();
        } break;
        case sequencer_SUN_WAVE_ORANGE: {
            // calculate_step_mounting_sun_waves_orange_next();
            calculate_step__wave4_next(tail_sun_count);
        } break;
    }
}



void update() {
    if (sequencer_mode != sequencer_OFF) {
        // update as often as possible
        calculate_step();
        // onlny switch to next step if time is over
        if ((millis() - sequencer_timestamp_last) > sequencer_interval) {
            sequencer_timestamp_last =  millis();
            calculate_step_next();
        }
    }
}


}  // namespace effect_engine