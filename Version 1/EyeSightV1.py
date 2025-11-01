from PIL import Image
import pytesseract
import pyttsx3
import string
from picamera import PiCamera
from time import sleep
from spellchecker import SpellChecker
from gpiozero import Button, LED
import sys
from googletrans import Translator

destinationLanguage = "en" # en is english
destinationLanguageFull = "english" # for speaking numbers

translator = Translator()

def Translate(txt):
    
    return translator.translate(txt, dest=destinationLanguage).text



def Speak(txt) :

    engine = pyttsx3.init()
    engine.setProperty("voice", destinationLanguageFull)
    engine.setProperty("rate", 125)
    engine.setProperty("volume", 1.0)
    engine.say(txt)
    engine.runAndWait()
    return


def filterDef(variable):
    if variable.lower() in (list(string.ascii_lowercase + string.digits + ",. ")):
        return True
    else:
        return False
            
                                                                    
Speak("Ready to scan")

speller = SpellChecker()

led = LED(12)

ScanButton = Button(17)

OCRMode = 2 # 0 = general # 1 = currency # 2 = printed text # 3 = Handwriting


def ScanAndRead():

    try:
        camera = PiCamera()
        camera.rotation = 270
        
        camera.start_preview(alpha=155)
        led.on()

        sleep(1)

        for number in ["5", "4", "3", "2", "1"]:
            Speak(number)
            sleep(1)

        camera.capture("/home/pi/PyTests/test.png")
        camera.stop_preview()
        led.off()

        camera.close()

        Speak("Processing")
        
    except:
        Speak("Camera Capture failed")
        sys.exit()

    try:

        img = Image.open("/home/pi/PyTests/test.png")

        psmMode = 0
        minConf = 15

        if OCRMode == 1:
            psmMode = 7
            minConf = 50
        elif OCRMode == 2:
            psmMode = 6
            minConf = 75
        elif OCRMode == 3:
            psmMode = 12
            minConf = 15
        else:
            psmMode = 3
            minConf = 20
            
        print("B1")

        data = pytesseract.image_to_data(img, config='-l eng --oem 3 --psm ' + str(psmMode))

        print("B2")

        lines = data.split("\n")
        lines.pop(0)

        textarr = []

        ItemCount = 0

        for line in lines:

            columns = line.split("	")

            if len(columns) >= 12:
                conf = columns[10]
                name = columns[11]

                ItemCount += 1

                if int(conf) >= minConf:
                    textarr.append(name)

        print(ItemCount)

        if ItemCount == 0:

            Speak("No text detected. Try Again")

            sleep(3)
    
            Speak("Ready to scan")

            return

        text = " ".join(textarr)

        print(data)

        englishText = "".join(filter(filterDef, list(text)))

        spellCheckedText = ""
        Errors = 0

        sourceLang = "xyz"

        try:
            sourceLang = translator.detect(text).lang
        except:
            pass            

        if sourceLang == "en":
            for word in englishText.split(" "):

                LastChar = ""

                if len(list(word)) > 0:
                    if word[-1] in [",", "."]:
                        LastChar = word[-1]
                        word = word.rstrip(LastChar)

                    spellCheckedText += (speller.correction(word) + LastChar + " ")

                    if speller.correction(word) != word and word.isnumeric():
                        Errors += 1

            try:
                if sourceLang != destinationLanguage:
                    spellCheckedText = Translate(spellCheckedText)
                    englishText = spellCheckedText
            except:
                Speak("Internet connection issues - unable to translate")
                englishText = (spellCheckedText)

                
        else:
            try:
                if sourceLang != destinationLanguage:
                    englishText = Translate(text)
                    spellCheckedText = englishText
                else:
                    englishText = (text)
                    spellCheckedText = (text)
            except:
                Speak("Internet connection issues - unable to translate")
                englishText = (text)
                spellCheckedText = (text)
                sleep(1)
                        

        if len(englishText.split(" ")) == 0:

            Speak("No text detected. Try Again")

            sleep(3)
    
            Speak("Ready to scan")

            return
        


        ErrorPercent = float(Errors)/float(len(englishText.split(" "))) * 100
        print(ErrorPercent)

        UsagePercent = float(len(spellCheckedText.split(" ")))/float(ItemCount) * 100

        print(UsagePercent)

        print(spellCheckedText)


        if spellCheckedText.strip() == "":
            Speak("No Text Detected")
            print("No Text Detected")
        else:
            Speak(spellCheckedText)

    except:

        Speak("No text detected. Try Again")

    sleep(3)

    Speak("Ready to scan")


while True:
    try:
        ScanButton.wait_for_press()
        print("scanning")
        ScanAndRead()
    except:        # KeyboardInterrupt exception when Ctrl+C is pressed
        print("some error")
        break

print("Exiting")
sys.exit()
