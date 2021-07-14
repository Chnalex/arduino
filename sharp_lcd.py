#! /usr/bin/env python3
# coding: utf-8

import sys
#sys.path.append('../')
import RPi.GPIO as GPIO
import rgb1602
import time
import os
import subprocess
from subprocess import Popen, PIPE, STDOUT
os.system("echo 0 | sudo tee /sys/class/leds/led1/brightness")

lcd = rgb1602.RGB1602(16,2)
#blanc
#lcd.setRGB(100, 100, 100)
GPIO.setmode(GPIO.BCM)
# Define keys
lcd_key     = 0
key_in  = 0

btnRIGHT  = 0
btnUP     = 1
btnDOWN   = 2
btnLEFT   = 3
btnSELECT = 4


menu_pos = [ [[0,0,6],[0,8,15],[1,9,15]] ,  [[0,8,15]] , [[0,8,15]]  ]
alpha = ['_','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z']
type_pc = ['pc-1251','pc-1262','pc-1350','pc-1403']
menu_courant=1
nv_menu=1
index_menu=0

GPIO.setup(16, GPIO.IN)
GPIO.setup(17, GPIO.IN)
GPIO.setup(18, GPIO.IN)
GPIO.setup(19, GPIO.IN)
GPIO.setup(20, GPIO.IN)

type_charg=""

def change_curseur(direction):
      global index_menu
      if (direction == "droite"):
         efface_curseur()
         index_menu += 1
         tempo=len(menu_pos[menu_courant-1])-1
         if index_menu > tempo:
           index_menu=0
      if (direction == "gauche"):
         efface_curseur()
         index_menu -= 1
         if index_menu < 0:
           index_menu=len(menu_pos[menu_courant-1])-1
      if (direction == "haut"):
         toto=1
      if (direction == "bas"):
         toto=1
      affiche_curseur()


def efface_curseur():
   lcd.setCursor(menu_pos[menu_courant-1][index_menu][1],menu_pos[menu_courant-1][index_menu][0])
   lcd.printout(" ")
   lcd.setCursor(menu_pos[menu_courant-1][index_menu][2],menu_pos[menu_courant-1][index_menu][0])
   lcd.printout(" ")

def affiche_curseur():
   lcd.setCursor(menu_pos[menu_courant-1][index_menu][1],menu_pos[menu_courant-1][index_menu][0])
   lcd.printout("<")
   lcd.setCursor(menu_pos[menu_courant-1][index_menu][2],menu_pos[menu_courant-1][index_menu][0])
   lcd.printout(">")

#Read the key value
def read_LCD_buttons():
  key_in16 = GPIO.input(16)
  key_in17 = GPIO.input(17)
  key_in18 = GPIO.input(18)
  key_in19 = GPIO.input(19)
  key_in20 = GPIO.input(20)
 
  if (key_in16 == 1):
    return btnSELECT
  if (key_in17 == 1):
    return btnUP
  if (key_in18 == 1):
    return btnDOWN
  if (key_in19 == 1):
    return btnLEFT
  if (key_in20 == 1):
    return btnRIGHT


def couleur_fond():
        if  (menu_courant==1):
           lcd.setRGB(100, 100, 100)
        if  (menu_courant==2):
           lcd.setRGB(40, 120, 200)
        if  (menu_courant==3 or menu_courant==4 or menu_courant==5 or menu_courant==6 or menu_courant==7 or menu_courant==8):
           lcd.setRGB(125,50,0)
        if  (menu_courant==9 or menu_courant==10):
           lcd.setRGB(0, 100,0)

def raz_ligne2():
           lcd.setCursor(0,1)
           lcd.printout("                ")

def raz_ligne1():
           lcd.setCursor(0,0)
           lcd.printout("                ")


def affiche_menu():
        raz_ligne1()
        raz_ligne2()
        couleur_fond()
        if  (menu_courant==1):
           lcd.setCursor(0,0)
           lcd.printout(" Sauv.   Charg. ")
           lcd.setCursor(0,1)
           lcd.printout("          Arret ")
        if  (menu_courant==2):
           lcd.setCursor(0,0)
           lcd.printout("         Retour ")
           lcd.setCursor(0,1)
           lcd.printout("                ")
        if  (menu_courant==3):
           lcd.setCursor(0,0)
           lcd.printout("         Retour ")
           lcd.setCursor(0,1)
           lcd.printout("                ")


def test_cle_usb():
   print('testcleusb')
   raz_ligne2()
   lcd.setCursor(0,1)
   lcd.printout("Recherche cle ")
   time.sleep(1)
   #recherche montage clé usb
   cmd="mount | grep -c  /media/usb0"
   proc = subprocess.Popen(cmd, shell=True, stdin=PIPE, stdout=PIPE, stderr=STDOUT, close_fds=True)
   output = proc.stdout.readline()
   line = output.decode().strip('\n')
   print(line)
   if line=='0':
       raz_ligne2()
       lcd.setCursor(0,1)
       lcd.printout("Cle absente !!  ")
   else:
       if  (menu_courant==2):
           gestion_sauvegarde()
       if  (menu_courant==3):
           gestion_chargement()


def  gestion_sauvegarde():
   global alpha
   global nv_menu
   global type_pc
   raz_ligne2()
   lcd.setCursor(0,1)
   lcd.printout("Cle OK")
   lcd.setCursor(0,0)
   lcd.printout("         Retour ")
   time.sleep(1)
   raz_ligne2()
   lcd.setCursor(0,0)
   lcd.printout("Nom?")
   compteur=0
   lettre=0
   chaine=''
   lcd.setCursor(0,1)
   lcd.printout(alpha[lettre])
   while compteur<5:
     lcd_key = read_LCD_buttons()
     if (lcd_key == btnRIGHT):
        lettre=lettre+1
        if lettre>26:
         lettre=0
        lcd.setCursor(compteur,1)
        lcd.printout(alpha[lettre])
        time.sleep(0.5)
     elif (lcd_key == btnLEFT):
        lettre=lettre-1
        if lettre<0:
         lettre=26
        lcd.setCursor(compteur,1)
        lcd.printout(alpha[lettre])
        time.sleep(0.5)
     elif (lcd_key == btnSELECT):
        chaine=chaine+alpha[lettre]
        compteur=compteur+1
        lcd.setCursor(compteur,1)
        lcd.printout("_")
        lettre=0
        time.sleep(0.4)
   print(chaine)
   lcd.setCursor(12,1)
   lcd.printout("<OK>")
   lcd.setCursor(8,0)
   lcd.printout(" Retour ")
   choix=0
   while True:
     lcd_key = read_LCD_buttons()
     if (lcd_key == btnRIGHT):
        lcd.setCursor(8,0)
        lcd.printout("<Retour>")
        lcd.setCursor(12,1)
        lcd.printout(" OK ")
        choix=1
        time.sleep(0.5)
     elif (lcd_key == btnLEFT):
        lcd.setCursor(8,0)
        lcd.printout(" Retour ")
        lcd.setCursor(12,1)
        lcd.printout("<OK>")
        choix=0
        time.sleep(0.5)
     elif (lcd_key == btnSELECT):
        time.sleep(0.4)
        break
   if choix==0:
      compteur=0
      raz_ligne2()
      lcd.setCursor(0,1)
      lcd.printout(type_pc[compteur])
      type=type_pc[compteur]
      type=type[3:len(type)]
      while True:
        lcd_key = read_LCD_buttons()
        if (lcd_key == btnRIGHT):
           compteur=compteur+1
           if compteur>3:
            compteur=0
           raz_ligne2()
           lcd.setCursor(0,1)
           lcd.printout(type_pc[compteur])
           type=type_pc[compteur]
           type=type[3:len(type)]
           time.sleep(0.5)
        elif (lcd_key == btnLEFT):
           compteur=compteur-1
           if compteur<0:
            compteur=3
           raz_ligne2()
           lcd.setCursor(0,1)
           lcd.printout(type_pc[compteur])
           type=type_pc[compteur]
           type=type[3:len(type)]
           time.sleep(0.5)
        elif (lcd_key == btnSELECT):
           print(type)
           time.sleep(0.4)
           break
      raz_ligne2()
      raz_ligne1()
      lcd.setCursor(0,1)
      lcd.printout("En cours...")
      commande="cd /media/usb0;csave -x -p " + type + " -c 1 -d 0 " + chaine + ".bas" 
      print(commande)
      os.system(commande)
      lcd.setCursor(0,1)
      lcd.printout("Fini !!     ")
      lcd.setCursor(8,0)
      lcd.printout("<Retour>")
   else:
      nv_menu=1
      recharge_menu()

def gestion_chargement():
  global type_pc
  global nv_menu
  chemin='/media/usb0'
  list_fichiers = [f for f in os.listdir(chemin) if f.endswith('.bas')]
  print(list_fichiers)
  lcd.setCursor(0,0)
  lcd.printout("Fich.?          ")
  raz_ligne2()
  compteur=0
  lcd.setCursor(0,1)
  nom_court=list_fichiers[compteur]
  nom_court=nom_court[0:5]+"..."
  lcd.printout(nom_court)
  while True:
        lcd_key = read_LCD_buttons()
        if (lcd_key == btnRIGHT):
           compteur=compteur+1
           if compteur>len(list_fichiers)-1:
            compteur=0
           raz_ligne2()
           lcd.setCursor(0,1)
           nom_court=list_fichiers[compteur]
           nom_court=nom_court[0:5]+"..."
           lcd.printout(nom_court)
           time.sleep(0.5)
        elif (lcd_key == btnLEFT):
           compteur=compteur-1
           if compteur<0:
            compteur=len(list_fichiers)-1
           raz_ligne2()
           lcd.setCursor(0,1)
           nom_court=list_fichiers[compteur]
           nom_court=nom_court[0:5]+"..."
           lcd.printout(nom_court)
           time.sleep(0.5)
        elif (lcd_key == btnSELECT):
           print(nom_court)
           time.sleep(0.2)
           break

  nom_fichier=list_fichiers[compteur]
  print(nom_fichier)
  raz_ligne1()
  raz_ligne2()
  compteur=0
  lcd.setCursor(0,1)
  lcd.printout(type_pc[compteur])
  type=type_pc[compteur]
  type=type[3:len(type)]
  time.sleep(0.3)
  while True:
        lcd_key = read_LCD_buttons()
        if (lcd_key == btnRIGHT):
           compteur=compteur+1
           if compteur>3:
            compteur=0
           raz_ligne2()
           lcd.setCursor(0,1)
           lcd.printout(type_pc[compteur])
           type=type_pc[compteur]
           type=type[3:len(type)]
           time.sleep(0.5)
        elif (lcd_key == btnLEFT):
           compteur=compteur-1
           if compteur<0:
            compteur=3
           raz_ligne2()
           lcd.setCursor(0,1)
           lcd.printout(type_pc[compteur])
           type=type_pc[compteur]
           type=type[3:len(type)]
           time.sleep(0.5)
        elif (lcd_key == btnSELECT):
           print(type)
           time.sleep(0.2)
           break
  raz_ligne2()
  lcd.setCursor(8,0)
  lcd.printout(" Retour ")
  lcd.setCursor(0,1)
  lcd.printout("Pret ?    <ok> ")
  choix=1
  while True:
     lcd_key = read_LCD_buttons()
     if (lcd_key == btnRIGHT):
        lcd.setCursor(8,0)
        lcd.printout("<Retour>")
        lcd.setCursor(0,1)
        lcd.printout("Pret ?     ok  ")
        choix=0
        time.sleep(0.5)
     elif (lcd_key == btnLEFT):
        lcd.setCursor(8,0)
        lcd.printout(" Retour ")
        lcd.setCursor(0,1)
        lcd.printout("Pret ?    <ok> ")
        choix=1
        time.sleep(0.5)
     elif (lcd_key == btnSELECT):
        time.sleep(0.4)
        break
  if choix==1:
     raz_ligne2()
     lcd.setCursor(0,1)
     lcd.printout("En cours...")
     commande="cd /media/usb0;cload -x -s -p " + type + " -c 1 -d 0 " + nom_fichier
     print(commande)
     os.system(commande)
     lcd.setCursor(0,1)
     lcd.printout("Fini !!     ")
     lcd.setCursor(8,0)
     lcd.printout("<Retour>")
  else:
     nv_menu=1
     recharge_menu()


def arret_raspi():
  raz_ligne1()
  raz_ligne2()
  lcd.setRGB(200, 0, 0)
  lcd.setCursor(0,0)
  lcd.printout(" Arret....")
  lcd.setCursor(0,1)
  lcd.printout("Attente 1min !")
  #time.sleep(2)
  #lcd.setRGB(0, 0, 0)
  os.system("sudo shutdown -h now")




def action_menu():
        global type_charg
        global nv_menu
        if  (menu_courant==1):
                  #menu debut: sauv, arret, charge
                  if (index_menu==0):
                       nv_menu=2
                       recharge_menu()
                       #sauv -> menu3
                       test_cle_usb()
                       return
                  if (index_menu==1):
                       #charger -> menu9
                       nv_menu=3
                       recharge_menu()
                       test_cle_usb()
                       return
                  if (index_menu==2):
                       arret_raspi()
                       return
        if  (menu_courant==2):
                  #sauv
                  if (index_menu==0):
                       nv_menu=1
                       recharge_menu()
                       return
        if  (menu_courant==3):
                  #charger
                  if (index_menu==0):
                       nv_menu=1
                       recharge_menu()
                       return

def recharge_menu():
  global menu_courant
  global nv_menu
  global index_menu
  if (menu_courant!=nv_menu):
     print('menu changé')
     menu_courant=nv_menu
     index_menu=0
     affiche_menu()
     affiche_curseur()
     

affiche_menu()
affiche_curseur()
while True:
  time.sleep(0.15)
  lcd_key = read_LCD_buttons()
  recharge_menu()

  if (lcd_key == btnRIGHT):
     change_curseur("droite")
     time.sleep(0.5)
  elif (lcd_key == btnLEFT):
     change_curseur("gauche")
     time.sleep(0.5)
  elif (lcd_key == btnUP):
     change_curseur("haut")
     time.sleep(0.5)
  elif (lcd_key == btnDOWN):
     change_curseur("bas")
     time.sleep(0.5)
  elif (lcd_key == btnSELECT):
     action_menu()
     time.sleep(0.4)
   
