/*
Copyright Production 3000

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// #ifndef XMODULELED_PIXELGROUP
// #define XMODULELED_PIXELGROUP

// struct PixelGroup {
//     PixelGroup(uint8_t ledCount, uint8_t start, uint8_t end) : ledCount(ledCount) {
//         bitarray[0] = 0;
//         bitarray[1] = 0;
//         bitarray[2] = 0;
//         bitarray[3] = 0;

//         for (uint8_t i = start; i <= end; i++) {
//             setBit(i, 1);
//         }
//     }

//     uint8_t ledCount;

//     char bitarray[4]; // since 4*8 this array actually contains 32 bits

//     char getBit(int index) {
//         return (bitarray[index/8] >> 7-(index & 0x7)) & 0x1;
//     }

//     void setBit(int index, int value) {
//         bitarray[index/8] = bitarray[index/8] | (value & 0x1) << 7-(index & 0x7);
//     }
// };



    // PixelGroup* pixelGroup;



    // pixelGroup = new PixelGroup(cfgXmoduleLED.ledCount, 0, cfgXmoduleLED.ledCount - 1);
    // pixelGroup = new PixelGroup(cfgXmoduleLED.ledCount, 0, 5);


// #endif