# signtospeech_convertor
### Project SSC - Sign to Speech convertor

SSC is a device that can read the signs and gestures of a person and convert it to simple speech. Mute people are usually acquainted with sign language and that acts as input to the system. 


The signs, based on finger movements and angle of the hands are decoded to specific words using a Deep Learning model deployed on a microcontroller, which is further fed to a speaker. Custom dataset was created for the same and the entire system is compact, portable, light-weight and power efficient.


### Quick links
- [Project documentation](https://drive.google.com/file/d/1hU0r5dFyuELzkhhjKlhBQMvRDwIzA0_e/view?usp=sharing)
- [Demonstration video](https://drive.google.com/file/d/1FI9mnAzsH2FQj-pWMHhiXKCZLUI9PEli/view?usp=sharing)
- [Project promo video](https://youtu.be/4ujX_pGNzEo)
- [Project features video](https://youtu.be/oq0OR4O_MMM)

### Contents
- [Problem statement](#problem-statement)
- [Proposed solution](#proposed-solution)
- [Technical explanation](#technical-explanation)
- [Flowchart](#flowchart)
- [Novelty](#novelty)
- [Gallery](#gallery)
- [Team SSC](#team-ssc)


## Problem statement
People who are mute or have difficulties in talking rely on sign language and gestures to communicate. This becomes really difficult when the communication happens between a mute person and a normal person, who is not accustomed to sign language. The person has to therefore use simpler gestures or write down to communicate which makes it tedious, slow and is not suitable in different situations. This issue is not only
applicable for people who lost their voice, but also people who undergo surgeries in their throat and cannot talk for prolonged periods of time.


We wanted to focus on this aspect and developed a mobile system that can actually help mute people in conveying their thoughts to the outside world, who don’t know or are not accustomed to the sign language system. 

## Proposed solution
Our solution to this involves a sign language to speech converter device that can read the signs and gestures of a person and convert it to simple speech. Mute people are usually acquainted with sign language and that acts as input to the system. The signs, based on finger movements and angle of the hands, can be decoded to specific words which are then fed to a speaker with a custom voice set. Hence it removes the barrier of proper communication between such people and normally enabled humans. The system will also be tuned and calibrated to a specific person according to hand and finger movements to improve speech, efficiency and accuracy.

## Technical explanation
Our device uses signs and gestures as input. We use flex sensors and inertial measurement units (IMU) to properly obtain the input signals. These input signals are used to decode the word/content for communication. For tracking gestures, the IMU data will be used to predict the angle of the hand. The data/content predicted is then fed to audio output based on pre-set voice. The entire system is battery powered making it mobile. The main aim is to process everything quickly and make maximum predictions possible compared to contemporary designs and devices.

## Novelty
The proposed solution involves a pair of lightweight gloves equipped with nonintrusive sensors which enhances user experience and convenience. This solution avoids use of expensive high-resolution cameras which are used in traditional visionbased gesture recognition systems. The hand gesture recognition model is run on a microcontroller, whereas traditional solutions involve much powerful and expensive
microprocessor-based systems like Raspberry Pi. Use of a microcontroller in place of a microprocessor-based system is advantageous since it has reduced power consumption, occupies less space and is comparatively lighter.

## Team SSC
### Designed and Developed by
- [Aswin Sreekumar](https://github.com/aswin-sreekumar)
- [Satish Kumar L](https://github.com/Satish-Kumar-L)
- [Greeshwar R S](https://github.com/greesh02)

### Additional credits
#### Custom case CAD design
- [Jai Kesav K R](https://github.com/nice-boi-jk)

#### Technical support
- [Shrikumaran P B](https://github.com/shrikumaran)
- [Kailash Jagadeesh](https://github.com/kailashjagadeesh)

## Flowchart
#### Hardware
![submission_hw](https://user-images.githubusercontent.com/63254914/171636579-ad1b4889-9682-4473-91a8-ac6a6bbb3f2e.png)

#### Software - sPresense utility box
![submission_sw](https://user-images.githubusercontent.com/63254914/171636585-a291e7e6-ccde-4bef-8e61-95c34d534054.png)

#### Software - Glove subsystem
![glove_sw_original](https://user-images.githubusercontent.com/63254914/171636693-4190d7e0-aa49-4e4b-8420-d2d9a8695e43.png)

## Gallery
![Screenshot (1305)](https://user-images.githubusercontent.com/63254914/171638198-7defc559-024b-4c9f-be06-4561a72d2889.png)
![20220315_232101](https://user-images.githubusercontent.com/63254914/171638214-81f5b721-0836-4bd3-a23f-4998412a62fc.jpg)
![20220228_175333](https://user-images.githubusercontent.com/63254914/171639261-322c1ffd-1ecf-4c0d-9119-365631da6035.jpg)
![20220128_200702](https://user-images.githubusercontent.com/63254914/171639273-484bb18f-6904-4d4a-bd20-fc77d1b995fd.jpg)
![20220128_191528](https://user-images.githubusercontent.com/63254914/171639281-c3f30a2d-decf-4704-8f97-1cf177306c25.jpg)
