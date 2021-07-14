#!/usr/bin/env python


import serial
import string
import pygame
import time 
import os
import sys

#fin definition variables globales et fonctions

#utilisaton du framebuffer pour la sortie video et touchscreen
os.environ['SDL_VIDEODRIVER'] = "fbcon"
os.environ["SDL_FBDEV"] = "/dev/fb0" 
os.environ["SDL_MOUSEDRV"] = "TSLIB"
os.environ["SDL_MOUSEDEV"] = "/dev/input/event1"


#definition des zones des boutons de l'accueil
bouton_acceuil_1=pygame.Rect(27, 82, 78, 60)
bouton_acceuil_2=pygame.Rect(27, 151, 78, 60)
bouton_acceuil_3=pygame.Rect(27, 220, 78, 60)
bouton_acceuil_4=pygame.Rect(139, 82, 78, 60)
bouton_acceuil_5=pygame.Rect(139, 151, 78, 60)
bouton_acceuil_6=pygame.Rect(139, 220, 78, 60)
bouton_acceuil_7=pygame.Rect(250, 82, 78, 60)
bouton_acceuil_8=pygame.Rect(250, 151, 78, 60)
bouton_acceuil_9=pygame.Rect(250, 220, 78, 60)
bouton_acceuil_10=pygame.Rect(361, 82, 78, 60)
bouton_acceuil_11=pygame.Rect(361, 151, 78, 60)
bouton_acceuil_12=pygame.Rect(361, 220, 78, 60)
bouton_acceuil_13=pygame.Rect(472, 82, 78, 60)
bouton_acceuil_14=pygame.Rect(472, 151, 78, 60)
bouton_acceuil_15=pygame.Rect(472, 220, 78, 60)
bouton_acceuil_16=pygame.Rect(583, 82, 78, 60)
bouton_acceuil_17=pygame.Rect(583, 151, 78, 60)
bouton_acceuil_18=pygame.Rect(583, 220, 78, 60)
bouton_acceuil_19=pygame.Rect(694, 82, 78, 60)
bouton_acceuil_20=pygame.Rect(694, 151, 78, 60)
bouton_acceuil_21=pygame.Rect(694, 220, 78, 60)
bouton_acceuil_22=pygame.Rect(805, 82, 78, 60)
bouton_acceuil_23=pygame.Rect(805, 151, 78, 60)
bouton_acceuil_24=pygame.Rect(805, 220, 78, 60)
bouton_acceuil_25=pygame.Rect(916, 82, 78, 60)
bouton_acceuil_26=pygame.Rect(916, 151, 78, 60)
bouton_acceuil_27=pygame.Rect(916, 220, 78, 60)
bouton_acceuil_28=pygame.Rect(256, 343, 78, 60)
bouton_acceuil_29=pygame.Rect(341, 343, 78, 60)
bouton_acceuil_30=pygame.Rect(256, 411, 78, 60)
bouton_acceuil_31=pygame.Rect(341, 411, 78, 60)
bouton_acceuil_32=pygame.Rect(427, 341, 78, 60)
bouton_acceuil_33=pygame.Rect(427, 407, 78, 60)
bouton_acceuil_34=pygame.Rect(427, 506, 78, 60)
bouton_acceuil_35=pygame.Rect(916, 505, 78, 60)

#definition des zones boutons de la programmation des volets
bouton_progvolet_1=pygame.Rect(31, 247, 76 , 53)
bouton_progvolet_2=pygame.Rect(31, 320, 76 , 60)
bouton_progvolet_3=pygame.Rect(31, 385, 76 , 60)
bouton_progvolet_4=pygame.Rect(31, 451, 76 , 60)
bouton_progvolet_5=pygame.Rect(142, 247, 76 , 53)
bouton_progvolet_6=pygame.Rect(142, 320, 76 , 60)
bouton_progvolet_7=pygame.Rect(142, 385, 76 , 60)
bouton_progvolet_8=pygame.Rect(142, 451, 76 , 60)
bouton_progvolet_9=pygame.Rect(253, 247, 76 , 53)
bouton_progvolet_10=pygame.Rect(253, 320, 76 , 60)
bouton_progvolet_11=pygame.Rect(253, 385, 76 , 60)
bouton_progvolet_12=pygame.Rect(253, 451, 76 , 60)
bouton_progvolet_13=pygame.Rect(364, 247, 76 , 53)
bouton_progvolet_14=pygame.Rect(364, 320, 76 , 60)
bouton_progvolet_15=pygame.Rect(364, 385, 76 , 60)
bouton_progvolet_16=pygame.Rect(364, 451, 76 , 60)
bouton_progvolet_17=pygame.Rect(475, 247, 76 , 53)
bouton_progvolet_18=pygame.Rect(475, 320, 76 , 60)
bouton_progvolet_19=pygame.Rect(475, 385, 76 , 60)
bouton_progvolet_20=pygame.Rect(475, 451, 76 , 60)
bouton_progvolet_21=pygame.Rect(586, 247, 76 , 53)
bouton_progvolet_22=pygame.Rect(586, 320, 76 , 60)
bouton_progvolet_23=pygame.Rect(586, 385, 76 , 60)
bouton_progvolet_24=pygame.Rect(586, 451, 76 , 60)
bouton_progvolet_25=pygame.Rect(697, 247, 76 , 53)
bouton_progvolet_26=pygame.Rect(697, 320, 76 , 60)
bouton_progvolet_27=pygame.Rect(697, 385, 76 , 60)
bouton_progvolet_28=pygame.Rect(697, 451, 76 , 60)
bouton_progvolet_29=pygame.Rect(808, 247, 76 , 53)
bouton_progvolet_30=pygame.Rect(808, 320, 76 , 60)
bouton_progvolet_31=pygame.Rect(808, 385, 76 , 60)
bouton_progvolet_32=pygame.Rect(808, 451, 76 , 60)
bouton_progvolet_33=pygame.Rect(919, 247, 76 , 53)
bouton_progvolet_34=pygame.Rect(919, 320, 76 , 60)
bouton_progvolet_35=pygame.Rect(919, 385, 76 , 60)
bouton_progvolet_36=pygame.Rect(919, 451, 76 , 60)
bouton_progvolet_37=pygame.Rect(476, 35, 75 , 58)

#definition des zones boutons de la programmation du chauffage
bouton_progchauff_1=pygame.Rect(279, 146, 70 , 50)
bouton_progchauff_4=pygame.Rect(279, 201, 70 , 50)
bouton_progchauff_7=pygame.Rect(279, 259, 70 , 50)
bouton_progchauff_10=pygame.Rect(279, 314, 70 , 50)
bouton_progchauff_13=pygame.Rect(279, 372, 70 , 50)
bouton_progchauff_16=pygame.Rect(279, 427, 70 , 50)
bouton_progchauff_19=pygame.Rect(279, 484, 70 , 50)
bouton_progchauff_2=pygame.Rect(575, 146, 70 , 50)
bouton_progchauff_5=pygame.Rect(575, 201, 70 , 50)
bouton_progchauff_8=pygame.Rect(575, 259, 70 , 50)
bouton_progchauff_11=pygame.Rect(575, 314, 70 , 50)
bouton_progchauff_14=pygame.Rect(575, 372, 70 , 50)
bouton_progchauff_17=pygame.Rect(575, 427, 70 , 50)
bouton_progchauff_20=pygame.Rect(575, 484, 70 , 50)
bouton_progchauff_3=pygame.Rect(871, 146, 70 , 50)
bouton_progchauff_6=pygame.Rect(871, 201, 70 , 50)
bouton_progchauff_9=pygame.Rect(871, 259, 70 , 50)
bouton_progchauff_12=pygame.Rect(871, 314, 70 , 50)
bouton_progchauff_15=pygame.Rect(871, 372, 70 , 50)
bouton_progchauff_18=pygame.Rect(871, 427, 70 , 50)
bouton_progchauff_21=pygame.Rect(871, 484, 70 , 50)
bouton_progchauff_22=pygame.Rect(474, 19, 76 , 60)


#list de stockage des horaires des volets en mode auto1
horaire_volet = [] 


#list de stockage des horaires des plages de chauffage
horaire_chauf = []

#ctrl-c


#fonction de sortie sur le port serie apres click dans le menu d'accueil
def appui_bouton_accueil(num_bouton):
 global ser
 #formatge de la chaine si appuie sur l'un des volets (haut, stop ou bas) 
 if num_bouton<28: 
    hbs=num_bouton%3
    volet=(num_bouton-hbs)/3+1
    if hbs==0:
     volet=(num_bouton-hbs)/3  
    chaine="av"+str(volet)
    if hbs==1:
      chaine+="h"
    if hbs==2:
      chaine+="s"
    if hbs==0:
      chaine+="b"      
    chaine+='\n'
    ser.write(chaine)
 if num_bouton==28:
    ser.write("ac-\n")
 if num_bouton==29:
    ser.write("ac+\n")
 if num_bouton==30:
    ser.write("aho\n")
 if num_bouton==31:
    ser.write("ahn\n")
 if num_bouton==32:
    ser.write("aco\n")
 if num_bouton==33:
    ser.write("acn\n")
 if num_bouton==34:
    ser.write("apc\n")
 if num_bouton==35:
    ser.write("apv\n")
    
 ser.flush()
 ser.flushInput()
 return;


#fonction de sortie sur le port serie apres click dans le menu prog volet + appel de la prog des volet en mode auto1
def appui_bouton_progvolet(num_bouton):
 global ser
 global horaire_volet
 #print ', '.join(map(str, horaire_volet))
 #formatage de la chaine si appuie sur l'un des volets (haut, stop ou bas) 
 if num_bouton<37:
    prg=num_bouton%4
    volet=(num_bouton-prg)/4+1
    if prg==0:
     volet=num_bouton/4
    chaine="pvm"+str(volet)
    ok=0
    for i in range(0, len(horaire_volet)):
     if horaire_volet[i]==volet:
       ok=1
       break
    if prg==1 and ok==1:
      prog_horaire(volet,1)
      return
    if prg==1:
      return
    if prg==2:
      chaine+="m"
    if prg==3:
      chaine+="h"
    if prg==0:
      chaine+="a"
    chaine+='\n'
    ser.write(chaine)
 if num_bouton==37:
    ser.write("pvr\n")
    
 ser.flush()
 ser.flushInput()
 return;



#fonction de sortie sur le port serie apres click dans le menu prog chauffage
def appui_bouton_progchauff(num_bouton):
 global ser
 if num_bouton<22:
    prog_horaire(num_bouton,2)
    return
 if num_bouton==22:
    ser.write("pcr\n")
    
 ser.flush()
 ser.flushInput()
 return;






#fonction de reglage sur la sortie video des horaires d'un volet en mode auto1 ou d'une plage de chauffage
# type=1 -> plage de horaire volet
# type=2 -> plage de chauffage
def prog_horaire(element,type):
 global menu_courant
 global cursor
 global horaire_volet
 global horaire_chauf
 global ser

 if type==1:
   for i in range(0, len(horaire_volet)):
     if horaire_volet[i]==element: 
       ouvr=horaire_volet[i+1]
       ferm=horaire_volet[i+2]
       break
   menu_courant="prog_volet_horaire"
   img=pygame.image.load('/domotique/plage_volet.png').convert()
 else:
   for i in range(0, 64):
     if horaire_chauf[i]==element:
       ouvr=horaire_chauf[i+1]
       ferm=horaire_chauf[i+2]
       break
   menu_courant="prog_chauf_horaire"
   img=pygame.image.load('/domotique/plage_chauf.png').convert()
 
 window.blit(img,(0,0))
 val1 = ouvr.split(":")
 val2 = ferm.split(":")
 heure_ouv=int(val1[0])
 minute_ouv=int(val1[1])
 heure_ferm=int(val2[0])
 minute_ferm=int(val2[1])
 mafont = pygame.font.Font("/usr/share/fonts/truetype/freefont/Arial_Bold.ttf", 50)
 heure_o = mafont.render(str(heure_ouv), 1, (200,100,15))
 window.blit(heure_o, (405,270))
 minute_o = mafont.render(str(minute_ouv), 1, (200,100,15))
 window.blit(minute_o, (685,270))
 heure_f = mafont.render(str(heure_ferm), 1, (200,100,15))
 window.blit(heure_f, (405,350))
 minute_f = mafont.render(str(minute_ferm), 1, (200,100,15))
 window.blit(minute_f, (685,350))
 pygame.display.flip()
 bouton_progplage_1=pygame.Rect(323, 266, 72 , 57)
 bouton_progplage_2=pygame.Rect(473, 266, 72 , 57)
 bouton_progplage_3=pygame.Rect(602, 266, 72 , 57)
 bouton_progplage_4=pygame.Rect(752, 266, 72 , 57)
 bouton_progplage_5=pygame.Rect(323, 347, 72 , 57)
 bouton_progplage_6=pygame.Rect(473, 347, 72 , 57)
 bouton_progplage_7=pygame.Rect(602, 347, 72 , 57)
 bouton_progplage_8=pygame.Rect(752, 347, 72 , 57)
 bouton_progplage_9=pygame.Rect(752, 170, 72 , 57)
 fin_boucle = False
 while not fin_boucle:
     for event in pygame.event.get():
      if event.type == pygame.MOUSEBUTTONUP and event.button == 1:
       #restauration du curseur par defaut quand le btn gauche de la souris est relache
       pygame.mouse.set_cursor(*pygame.cursors.arrow)
       pygame.mouse.set_visible(True)
      if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
       #modif du curseur (boule noire) lors d'un click gauche
       pygame.mouse.set_visible(True)
       pygame.mouse.set_cursor(*cursor)
       pos = pygame.mouse.get_pos()
       #gestion des collisions entre la souris et les zones cliquables en fx du type de fond (menu) affiche
       if bouton_progplage_1.collidepoint(pos):
          if heure_ouv==0:
           heure_ouv=23
          else: 
           heure_ouv -=1
          heure_o = mafont.render(str(heure_ouv), 1, (200,100,15))
          window.blit(img,(0,0))
          window.blit(heure_o, (405,270))
          pygame.display.update( pygame.Rect(399, 264, 70, 63) )
       if bouton_progplage_2.collidepoint(pos):
          if heure_ouv==23:
           heure_ouv=0
          else:
           heure_ouv +=1
          heure_o = mafont.render(str(heure_ouv), 1, (200,100,15))
          window.blit(img,(0,0))
          window.blit(heure_o, (405,270))
          pygame.display.update( pygame.Rect(399, 264, 70, 63) )
       if bouton_progplage_3.collidepoint(pos):
          if minute_ouv==0:
           minute_ouv=55
          else:
           minute_ouv -=5 
          minute_o = mafont.render(str(minute_ouv), 1, (200,100,15))
          window.blit(img,(0,0))
          window.blit(minute_o, (685,270))
          pygame.display.update( pygame.Rect(678, 264, 70, 63) )
       if bouton_progplage_4.collidepoint(pos):
          if minute_ouv==55:
           minute_ouv=0
          else:
           minute_ouv +=5
          minute_o = mafont.render(str(minute_ouv), 1, (200,100,15))
          window.blit(img,(0,0))
          window.blit(minute_o, (685,270))
          pygame.display.update( pygame.Rect(678, 264, 70, 63) )
       if bouton_progplage_5.collidepoint(pos):
          if heure_ferm==0:
           heure_ferm=23
          else:
           heure_ferm -=1
          heure_f = mafont.render(str(heure_ferm), 1, (200,100,15))
          window.blit(img,(0,0))
          window.blit(heure_f, (405,350))
          pygame.display.update( pygame.Rect(399, 345, 70, 63) )
       if bouton_progplage_6.collidepoint(pos):
          if heure_ferm==23:
           heure_ferm=0
          else:
           heure_ferm +=1
          heure_f = mafont.render(str(heure_ferm), 1, (200,100,15))
          window.blit(img,(0,0))
          window.blit(heure_f, (405,350))
          pygame.display.update( pygame.Rect(399, 345, 70, 63) )
       if bouton_progplage_7.collidepoint(pos):
          if minute_ferm==0:
           minute_ferm=55
          else:
           minute_ferm -=5
          minute_f= mafont.render(str(minute_ferm), 1, (200,100,15))
          window.blit(img,(0,0))
          window.blit(minute_f, (685,350))
          pygame.display.update( pygame.Rect(678, 345, 70, 63) )
       if bouton_progplage_8.collidepoint(pos):
          if minute_ferm==55:
           minute_ferm=0
          else:
           minute_ferm +=5
          minute_f= mafont.render(str(minute_ferm), 1, (200,100,15))
          window.blit(img,(0,0))
          window.blit(minute_f, (685,350))
          pygame.display.update( pygame.Rect(678, 345, 70, 63) )
       if bouton_progplage_9.collidepoint(pos):
          if type==1:
              chaine_s="pvp"
          else:
              chaine_s="pcp"
          chaine_s+=str(element)+"x"+str(heure_ouv)+":"+str(minute_ouv)+"."+str(heure_ferm)+":"+str(minute_ferm)+"\n"
          #print(chaine_s)
          ser.write(chaine_s)
          ser.flush()
          fin_boucle = True
          break 
 menu_courant=""
 ser.flushInput()
 return;


#fonction d'ajout dans la liste qui stocke par volet en mode auto1 l'horaire d'ouverture et de fermeture
def ajout_horaire_volet(volet,ouverture,fermeture):
 global horaire_volet
 horaire_volet.append(volet)
 horaire_volet.append(ouverture)
 horaire_volet.append(fermeture)
 #print ', '.join(map(str, horaire_volet))
 return;

#fonction de raz de la liste qui stocke par volet en mode auto1 l'horaire d'ouverture et de fermeture
def raz_horaire_volet():
 global horaire_volet
 horaire_volet = []
 return;




#fonction d'ajout dans la liste qui stocke par plage de chauffage l'horaire de debut et de fin
def ajout_horaire_chauf(plage,debut,fin):
 global horaire_chauf
 horaire_chauf.append(plage)
 horaire_chauf.append(debut)
 horaire_chauf.append(fin)
 #print ', '.join(map(str, horaire_chauf))
 return;

#fonction de raz de la liste qui stocke par volet en mode auto1 l'horaire d'ouverture et de fermeture
def raz_horaire_chauf():
 global horaire_chauf
 horaire_chauf = []
 return;


#
from signal import alarm, signal, SIGALRM, SIGKILL
def alarm_handler(signum, frame):
   raise Alarm


#fin definition variables globales et fonctions

#debut prog principal


#ouverture du port serie
ser = serial.Serial("/dev/ttyAMA0", baudrate=57600, timeout=0.5)
ser.open()
print( "Port serie: OK" )

#init pygame + taille fenetre de travail + fonts + curseur souris par defaut
pygame.init()
print( "Init pygame: OK" )
signal(SIGALRM, alarm_handler)
alarm(2)
try:
  window = pygame.display.set_mode()
  alarm(0)
  print("ok")
except Alarm:
  raise KeyboardInterrupt
myfont = pygame.font.Font("/usr/share/fonts/truetype/freefont/Arial_Bold.ttf", 20)
myfont1 = pygame.font.Font("/usr/share/fonts/truetype/freefont/Arial_Bold.ttf", 15)
img=pygame.image.load('/domotique/fond.png').convert()
window.blit(img,(0,0))
pygame.display.flip()
pygame.mouse.set_visible(False)
cursor = pygame.cursors.load_xbm('/domotique/firefox.xbm','/domotique/firefox_mask.xbm')
pygame.mouse.set_cursor(*pygame.cursors.arrow)

print( "Tout est ok, en attente des trames sur le port serie..." )

menu_courant=""
quit = False

while not quit:
  
  try:
#debut gestion affichage en fx du contenu des trames sur le port serie     
     time.sleep(0.05)
     line=""
     if ser.inWaiting() > 0 :     
        line += ser.read(280)
     words = line.split()

     
     if len(words)>0 and menu_courant!="prog_volet_horaire"  and menu_courant!="prog_chauf_horaire":
        if words[0] == "accueil":
		menu_courant="accueil"
		img=pygame.image.load('/domotique/accueil.png').convert()
		window.blit(img,(0,0))
                temp_bas = myfont.render(words[1], 1, (0,0,0))
                window.blit(temp_bas, (315, 490))
		temp_haut = myfont.render(words[2], 1, (0,0,0))
                window.blit(temp_haut, (315, 540))
		temp_cons = myfont.render(words[3], 1, (0,0,0))
                window.blit(temp_cons, (170, 360))
                relais_chauf= myfont1.render(words[10], 1, (167,93,13))
                window.blit(relais_chauf, (225, 385))		
                if words[4] == "on":
			img1=pygame.image.load('/root/domotique/on.png').convert()
                	window.blit(img1,(429,342))
		else:
                        img1=pygame.image.load('/domotique/off.png').convert()
                        window.blit(img1,(429,409))
                if words[5] == "oui":
                        img1=pygame.image.load('/domotique/oui.png').convert()
                        window.blit(img1,(259,413))
                else:
                        img1=pygame.image.load('/domotique/non.png').convert()
                        window.blit(img1,(342,413))
		datex = myfont.render(words[6], 1, (0,0,0))
                window.blit(datex, (830, 350))
		tempsx = myfont.render(words[7], 1, (0,0,0))
                window.blit(tempsx, (830, 385))
		ramx = myfont.render(words[8], 1, (0,0,0))
                window.blit(ramx, (830, 425))
		uptimex = myfont.render(words[9], 1, (0,0,0))
                window.blit(uptimex, (830, 460))		
		pygame.display.flip()
                fichier = open("/var/tmp/accueil.txt", "w")
                fichier.write(line)
                fichier.close()
                os.chmod("/var/tmp/accueil.txt", 0777)




        elif words[0] == "prog_volet":
		menu_courant="prog_volet"
                img=pygame.image.load('/domotique/prog_volet.png').convert()
                window.blit(img,(0,0))
                raz_horaire_volet()
   	        for i in range(0, 9):
                  if words[1+3*i] == "manu":
                        img1=pygame.image.load('/domotique/manu.png').convert_alpha()
                        window.blit(img1,(34+111*i,321))
                	heure_ouv = myfont1.render("--:--", 1, (200,100,15))
                	window.blit(heure_ouv, (55+111*i, 186))
			heure_ferm = myfont1.render("--:--", 1, (200,100,15))
                        window.blit(heure_ferm, (55+111*i, 220))
                  elif words[1+3*i] == "auto1":
                        img1=pygame.image.load('/domotique/auto1.png').convert_alpha()
                        window.blit(img1,(34+111*i,387))
                        img2=pygame.image.load('/domotique/modifier.png').convert_alpha()
                        window.blit(img2,(36+111*i,249))
                        heure_ouv = myfont1.render(words[2+3*i], 1, (200,100,15))
                        window.blit(heure_ouv, (55+111*i, 186))
                        heure_ferm = myfont1.render(words[3+3*i], 1, (200,100,15))
                        window.blit(heure_ferm, (55+111*i, 220))
                        ajout_horaire_volet(i+1,words[2+3*i],words[3+3*i])
		  elif words[1+3*i] == "auto2":           
                        img1=pygame.image.load('/domotique/auto2.png').convert_alpha()
                        window.blit(img1,(34+111*i,453))
                        heure_ouv = myfont1.render("--:--", 1, (200,100,15))
                        window.blit(heure_ouv, (55+111*i, 186))
                        heure_ferm = myfont1.render("--:--", 1, (200,100,15))
                        window.blit(heure_ferm, (55+111*i, 220))
		pygame.display.flip()     
                fichier = open("/var/tmp/prog_volet.txt", "w")
                fichier.write(line)
                fichier.close()



	elif words[0] == "prog_chauf":
                menu_courant="prog_chauf"
                img=pygame.image.load('/domotique/prog_chauf.png').convert()
                window.blit(img,(0,0))
		raz_horaire_chauf()
                for j in range(0, 7):
		   for i in range(0, 3):
			heure_deb = myfont1.render(words[1+2*i+6*j], 1, (200,100,15))
                	window.blit(heure_deb, (180+298*i, 163+56*j))
                        heure_fin = myfont1.render(words[2+2*i+6*j], 1, (200,100,15))
                        window.blit(heure_fin, (237+298*i, 163+56*j))
                        ajout_horaire_chauf(i+3*j+1,words[1+2*i+6*j],words[2+2*i+6*j])
		pygame.display.flip()
                fichier = open("/var/tmp/prog_chauf.txt", "w")
                fichier.write(line)
                fichier.close()

#fin traitement affichage


#debut traitement souris/touchscreen

     for event in pygame.event.get():
      if event.type == pygame.MOUSEBUTTONUP and event.button == 1:
       #restauration du curseur par defaut quand le btn gauche de la souris est relache
       pygame.mouse.set_cursor(*pygame.cursors.arrow)
       pygame.mouse.set_visible(False)
      if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
       #modif du curseur (boule noire) lors d'un click gauche
       pygame.mouse.set_visible(True)
       pygame.mouse.set_cursor(*cursor)
       pos = pygame.mouse.get_pos()
       #gestion des collisions entre la souris et les zones cliquables en fx du type de fond (menu) affiche
       if menu_courant=="accueil":
                if bouton_acceuil_1.collidepoint(pos):
                  appui_bouton_accueil(1)
                if bouton_acceuil_2.collidepoint(pos):
                  appui_bouton_accueil(2)
                if bouton_acceuil_3.collidepoint(pos):
                  appui_bouton_accueil(3)
                if bouton_acceuil_4.collidepoint(pos):
                  appui_bouton_accueil(4)
                if bouton_acceuil_5.collidepoint(pos):
                  appui_bouton_accueil(5)
                if bouton_acceuil_6.collidepoint(pos):
                  appui_bouton_accueil(6)
                if bouton_acceuil_7.collidepoint(pos):
                  appui_bouton_accueil(7)
                if bouton_acceuil_8.collidepoint(pos):
                  appui_bouton_accueil(8)
                if bouton_acceuil_9.collidepoint(pos):
                  appui_bouton_accueil(9)
                if bouton_acceuil_10.collidepoint(pos):
                  appui_bouton_accueil(10)
                if bouton_acceuil_11.collidepoint(pos):
                  appui_bouton_accueil(11)
                if bouton_acceuil_12.collidepoint(pos):
                  appui_bouton_accueil(12)
                if bouton_acceuil_13.collidepoint(pos):
                  appui_bouton_accueil(13)
                if bouton_acceuil_14.collidepoint(pos):
                  appui_bouton_accueil(14)
                if bouton_acceuil_15.collidepoint(pos):
                  appui_bouton_accueil(15)
                if bouton_acceuil_16.collidepoint(pos):
                  appui_bouton_accueil(16)
                if bouton_acceuil_17.collidepoint(pos):
                  appui_bouton_accueil(17)
                if bouton_acceuil_18.collidepoint(pos):
                  appui_bouton_accueil(18)
                if bouton_acceuil_19.collidepoint(pos):
                  appui_bouton_accueil(19)
                if bouton_acceuil_20.collidepoint(pos):
                  appui_bouton_accueil(20)
                if bouton_acceuil_21.collidepoint(pos):
                  appui_bouton_accueil(21)
                if bouton_acceuil_22.collidepoint(pos):
                  appui_bouton_accueil(22)
                if bouton_acceuil_23.collidepoint(pos):
                  appui_bouton_accueil(23)
                if bouton_acceuil_24.collidepoint(pos):
                  appui_bouton_accueil(24)
                if bouton_acceuil_25.collidepoint(pos):
                  appui_bouton_accueil(25)
                if bouton_acceuil_26.collidepoint(pos):
                  appui_bouton_accueil(26)
                if bouton_acceuil_27.collidepoint(pos):
                  appui_bouton_accueil(27)
                if bouton_acceuil_28.collidepoint(pos):
                  appui_bouton_accueil(28)
                if bouton_acceuil_29.collidepoint(pos):
                  appui_bouton_accueil(29)
                if bouton_acceuil_30.collidepoint(pos):
                  appui_bouton_accueil(30)
                if bouton_acceuil_31.collidepoint(pos):
                  appui_bouton_accueil(31)
                if bouton_acceuil_32.collidepoint(pos):
                  appui_bouton_accueil(32)
                if bouton_acceuil_33.collidepoint(pos):
                  appui_bouton_accueil(33)
                if bouton_acceuil_34.collidepoint(pos):
                  appui_bouton_accueil(34)
                if bouton_acceuil_35.collidepoint(pos):
                  appui_bouton_accueil(35)
       if menu_courant=="prog_volet":
                if bouton_progvolet_1.collidepoint(pos):
                    appui_bouton_progvolet(1)
                if bouton_progvolet_2.collidepoint(pos):
                    appui_bouton_progvolet(2)
                if bouton_progvolet_3.collidepoint(pos):
                    appui_bouton_progvolet(3)
                if bouton_progvolet_4.collidepoint(pos):
                    appui_bouton_progvolet(4)
                if bouton_progvolet_5.collidepoint(pos):
                    appui_bouton_progvolet(5)
                if bouton_progvolet_6.collidepoint(pos):
                    appui_bouton_progvolet(6)
                if bouton_progvolet_7.collidepoint(pos):
                    appui_bouton_progvolet(7)
                if bouton_progvolet_8.collidepoint(pos):
                    appui_bouton_progvolet(8)
                if bouton_progvolet_9.collidepoint(pos):
                    appui_bouton_progvolet(9)
                if bouton_progvolet_10.collidepoint(pos):
                    appui_bouton_progvolet(10)
                if bouton_progvolet_11.collidepoint(pos):
                    appui_bouton_progvolet(11)
                if bouton_progvolet_12.collidepoint(pos):
                    appui_bouton_progvolet(12)
                if bouton_progvolet_13.collidepoint(pos):
                    appui_bouton_progvolet(13)
                if bouton_progvolet_14.collidepoint(pos):
                    appui_bouton_progvolet(14)
                if bouton_progvolet_15.collidepoint(pos):
                    appui_bouton_progvolet(15)
                if bouton_progvolet_16.collidepoint(pos):
                    appui_bouton_progvolet(16)
                if bouton_progvolet_17.collidepoint(pos):
                    appui_bouton_progvolet(17)
                if bouton_progvolet_18.collidepoint(pos):
                    appui_bouton_progvolet(18)
                if bouton_progvolet_19.collidepoint(pos):
                    appui_bouton_progvolet(19)
                if bouton_progvolet_20.collidepoint(pos):
                    appui_bouton_progvolet(20)
                if bouton_progvolet_21.collidepoint(pos):
                    appui_bouton_progvolet(21)
                if bouton_progvolet_22.collidepoint(pos):
                    appui_bouton_progvolet(22)
                if bouton_progvolet_23.collidepoint(pos):
                    appui_bouton_progvolet(23)
                if bouton_progvolet_24.collidepoint(pos):
                    appui_bouton_progvolet(24)
                if bouton_progvolet_25.collidepoint(pos):
                    appui_bouton_progvolet(25)
                if bouton_progvolet_26.collidepoint(pos):
                    appui_bouton_progvolet(26)
                if bouton_progvolet_27.collidepoint(pos):
                    appui_bouton_progvolet(27)
                if bouton_progvolet_28.collidepoint(pos):
                    appui_bouton_progvolet(28)
                if bouton_progvolet_29.collidepoint(pos):
                    appui_bouton_progvolet(29)
                if bouton_progvolet_30.collidepoint(pos):
                    appui_bouton_progvolet(30)
                if bouton_progvolet_31.collidepoint(pos):
                    appui_bouton_progvolet(31)
                if bouton_progvolet_32.collidepoint(pos):
                    appui_bouton_progvolet(32)
                if bouton_progvolet_33.collidepoint(pos):
                    appui_bouton_progvolet(33)
                if bouton_progvolet_34.collidepoint(pos):
                    appui_bouton_progvolet(34)
                if bouton_progvolet_35.collidepoint(pos):
                    appui_bouton_progvolet(35)
                if bouton_progvolet_36.collidepoint(pos):
                    appui_bouton_progvolet(36)
                if bouton_progvolet_37.collidepoint(pos):
                    appui_bouton_progvolet(37)
       if menu_courant=="prog_chauf":
                if bouton_progchauff_1.collidepoint(pos):
                    appui_bouton_progchauff(1)
                if bouton_progchauff_2.collidepoint(pos):
                    appui_bouton_progchauff(2)
                if bouton_progchauff_3.collidepoint(pos):
                    appui_bouton_progchauff(3)
                if bouton_progchauff_4.collidepoint(pos):
                    appui_bouton_progchauff(4)
                if bouton_progchauff_5.collidepoint(pos):
                    appui_bouton_progchauff(5)
                if bouton_progchauff_6.collidepoint(pos):
                    appui_bouton_progchauff(6)
                if bouton_progchauff_7.collidepoint(pos):
                    appui_bouton_progchauff(7)
                if bouton_progchauff_8.collidepoint(pos):
                    appui_bouton_progchauff(8)
                if bouton_progchauff_9.collidepoint(pos):
                    appui_bouton_progchauff(9)
                if bouton_progchauff_10.collidepoint(pos):
                    appui_bouton_progchauff(10)
                if bouton_progchauff_11.collidepoint(pos):
                    appui_bouton_progchauff(11)
                if bouton_progchauff_12.collidepoint(pos):
                    appui_bouton_progchauff(12)
                if bouton_progchauff_13.collidepoint(pos):
                    appui_bouton_progchauff(13)
                if bouton_progchauff_14.collidepoint(pos):
                    appui_bouton_progchauff(14)
                if bouton_progchauff_15.collidepoint(pos):
                    appui_bouton_progchauff(15)
                if bouton_progchauff_16.collidepoint(pos):
                    appui_bouton_progchauff(16)
                if bouton_progchauff_17.collidepoint(pos):
                    appui_bouton_progchauff(17)
                if bouton_progchauff_18.collidepoint(pos):
                    appui_bouton_progchauff(18)
                if bouton_progchauff_19.collidepoint(pos):
                    appui_bouton_progchauff(19)
                if bouton_progchauff_20.collidepoint(pos):
                    appui_bouton_progchauff(20)
                if bouton_progchauff_21.collidepoint(pos):
                    appui_bouton_progchauff(21)
                if bouton_progchauff_22.collidepoint(pos):
                    appui_bouton_progchauff(22)
              
#fin traitement souris/touchscreen

#fin boucle infinie
  except:
      print "Oops!  sortie brutale du programme...." 
      pygame.quit()
      sys.exit()
ser.close()

pygame.quit()
exit(0)
