#!/usr/bin/python
# -*- coding:utf-8 -*-
import sys
import os
import png
import datetime
import logging
from waveshare_epd import epd4in2
import time
from PIL import Image,ImageDraw,ImageFont
import traceback
import json
import urllib2
from urllib2 import HTTPError
import locale
import unicodedata

logging.basicConfig(format='%(asctime)s %(message)s', datefmt='%d/%m/%Y %H:%M:%S %p',level=logging.DEBUG)

pic64 = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'pic/64x64')
pic128 = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'pic/128x128')
fontsdir = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'pic/fonts')
caldir = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'lib/fetes')


temp='--'
temp1='--'
temp2='--'
hum='--%'
vent='--km/h'
direction='--'
press='----Hpa'
date='---------- '
saint='-------'
ephemeride='lever: -h--  coucher: --h-- '
actu='unknown.bmp'
plus6='unknown.bmp'
plus12='unknown.bmp'
plus18='unknown.bmp'
maj='maj --h'
temp6='--'
hum6='--%'
vent6='--'
direction6='--'
temp12='--'
hum12='--%'
vent12='--'
direction12='--'
temp18='--'
hum18='--%'
vent18='--'
direction18='--'

locale.setlocale(locale.LC_TIME,'')
now = datetime.datetime.now()
current_time = now.strftime('%H:%M')
current_date= now.strftime('%A %d %B ') 
logging.info(current_date)
mois = int(now.strftime("%m"))
jour = int(now.strftime("%d"))
maj='maj '+current_time
logging.info(current_time)
date=unicode(current_date,'utf-8')


try:
    with open(os.path.join(caldir, 'calendrier.json'), "r") as f:
       calendrier = json.loads(f.read())
    saint0=calendrier['mois'][mois-1]['jour'][jour-1]['fete']
    saint0=saint0.encode('utf-8')
    saint=unicode(saint0,'utf-8')
    logging.info(saint0)

except IOError as e:
    logging.info(e)


try:
    url = "http://api.openweathermap.org/data/2.5/forecast?id=3037854&lang=fr&appid=e896c55da915daa3f71087cc8d950460&units=metric"
    response = urllib2.urlopen(url, timeout=5)
    data = json.loads(response.read())
    logging.info("definition variables dynamiques:")
    #temp=str(round(data['list'][0]['main']['temp']))+'C'
    #logging.info(temp)
    #hum=str(data['list'][0]['main']['humidity'])+'%'
    logging.info(hum)
    vent=str(int(round(3.6*data['list'][0]['wind']['speed'])))+'kmh'
    logging.info(vent)
    logging.info('dir')
    logging.info(str(data['list'][0]['wind']['deg']))
    if data['list'][0]['wind']['deg']>=337:
       direction='N'
    if data['list'][0]['wind']['deg']>=0 and data['list'][0]['wind']['deg']<22:
       direction='N'
    if data['list'][0]['wind']['deg']>=22 and data['list'][0]['wind']['deg']<67:
       direction='NE'
    if data['list'][0]['wind']['deg']>=67 and data['list'][0]['wind']['deg']<112:
       direction='E'  
    if data['list'][0]['wind']['deg']>=112 and data['list'][0]['wind']['deg']<157:
       direction='SE'
    if data['list'][0]['wind']['deg']>=157 and data['list'][0]['wind']['deg']<202:
       direction='S'       
    if data['list'][0]['wind']['deg']>=202 and data['list'][0]['wind']['deg']<247:
       direction='SO'
    if data['list'][0]['wind']['deg']>=247 and data['list'][0]['wind']['deg']<292:
       direction='O'
    if data['list'][0]['wind']['deg']>=292 and data['list'][0]['wind']['deg']<337:
       direction='N0'   
    logging.info(direction)   
    press=str(data['list'][0]['main']['pressure'])+'Hpa'
    logging.info(press)
    lever = int(data['city']['sunrise']+data['city']['timezone'])
    coucher = int(data['city']['sunset']+data['city']['timezone'])
    ephemeride='lever: '+datetime.datetime.utcfromtimestamp(lever).strftime('%H:%M')+'  coucher: '+datetime.datetime.utcfromtimestamp(coucher).strftime('%H:%M')+' '
    logging.info(ephemeride)
    actu=data['list'][0]['weather'][0]['icon']+'.bmp'
    logging.info(actu)
    plus6=data['list'][2]['weather'][0]['icon']+'.bmp'
    logging.info(plus6)
    plus12=data['list'][4]['weather'][0]['icon']+'.bmp'
    logging.info(plus12)
    plus18=data['list'][6]['weather'][0]['icon']+'.bmp'
    logging.info(plus18)
    temp6=str(int(round(data['list'][2]['main']['temp'])))+'C'
    logging.info(temp6)
    hum6=str(data['list'][2]['main']['humidity'])+'%'
    logging.info(hum6)
    vent6=str(int(round(3.6*data['list'][2]['wind']['speed'])))
    logging.info(vent6)
    logging.info('dir')
    logging.info(str(data['list'][2]['wind']['deg']))
    if data['list'][2]['wind']['deg']>=337:
       direction6='N'
    if data['list'][2]['wind']['deg']>=0 and data['list'][2]['wind']['deg']<22:
       direction6='N'
    if data['list'][2]['wind']['deg']>=22 and data['list'][2]['wind']['deg']<67:
       direction6='NE'
    if data['list'][2]['wind']['deg']>=67 and data['list'][2]['wind']['deg']<112:
       direction6='E'  
    if data['list'][2]['wind']['deg']>=112 and data['list'][2]['wind']['deg']<157:
       direction6='SE'
    if data['list'][2]['wind']['deg']>=157 and data['list'][2]['wind']['deg']<202:
       direction6='S'       
    if data['list'][2]['wind']['deg']>=202 and data['list'][2]['wind']['deg']<247:
       direction6='SO'
    if data['list'][2]['wind']['deg']>=247 and data['list'][2]['wind']['deg']<292:
       direction6='O'
    if data['list'][2]['wind']['deg']>=292 and data['list'][2]['wind']['deg']<337:
       direction6='N0'  
    logging.info(direction6)
    temp12=str(int(round(data['list'][4]['main']['temp'])))+'C'
    logging.info(temp12)
    hum12=str(data['list'][4]['main']['humidity'])+'%'
    logging.info(temp12)
    vent12=str(int(round(3.6*data['list'][4]['wind']['speed'])))
    logging.info(vent12)
    logging.info('dir')
    logging.info(str(data['list'][4]['wind']['deg']))
    if data['list'][4]['wind']['deg']>=337:
       direction12='N'
    if data['list'][4]['wind']['deg']>=0 and data['list'][4]['wind']['deg']<22:
       direction12='N'
    if data['list'][4]['wind']['deg']>=22 and data['list'][4]['wind']['deg']<67:
       direction12='NE'
    if data['list'][4]['wind']['deg']>=67 and data['list'][4]['wind']['deg']<112:
       direction12='E'  
    if data['list'][4]['wind']['deg']>=112 and data['list'][4]['wind']['deg']<157:
       direction12='SE'
    if data['list'][4]['wind']['deg']>=157 and data['list'][4]['wind']['deg']<202:
       direction12='S'       
    if data['list'][4]['wind']['deg']>=202 and data['list'][4]['wind']['deg']<247:
       direction12='SO'
    if data['list'][4]['wind']['deg']>=247 and data['list'][4]['wind']['deg']<292:
       direction12='O'
    if data['list'][4]['wind']['deg']>=292 and data['list'][4]['wind']['deg']<337:
       direction12='N0'  
    logging.info(direction12)
    temp18=str(int(round(data['list'][6]['main']['temp'])))+'C'
    logging.info(temp18)
    hum18=str(data['list'][6]['main']['humidity'])+'%'
    logging.info(hum18)
    vent18=str(int(round(3.6*data['list'][6]['wind']['speed'])))
    logging.info(vent18)
    logging.info('dir')
    logging.info(str(data['list'][6]['wind']['deg']))
    if data['list'][6]['wind']['deg']>=337:
       direction18='N'
    if data['list'][6]['wind']['deg']>=0 and data['list'][6]['wind']['deg']<22:
       direction18='N'
    if data['list'][6]['wind']['deg']>=22 and data['list'][6]['wind']['deg']<67:
       direction18='NE'
    if data['list'][6]['wind']['deg']>=67 and data['list'][6]['wind']['deg']<112:
       direction18='E'  
    if data['list'][6]['wind']['deg']>=112 and data['list'][6]['wind']['deg']<157:
       direction18='SE'
    if data['list'][6]['wind']['deg']>=157 and data['list'][6]['wind']['deg']<202:
       direction18='S'       
    if data['list'][6]['wind']['deg']>=202 and data['list'][6]['wind']['deg']<247:
       direction18='SO'
    if data['list'][6]['wind']['deg']>=247 and data['list'][6]['wind']['deg']<292:
       direction18='O'
    if data['list'][6]['wind']['deg']>=292 and data['list'][6]['wind']['deg']<337:
       direction18='N0'  
    logging.info(direction18)
except urllib2.HTTPError, e:
    logging.info(e.code)
    maj=str(e.code)
except urllib2.URLError, e:
    logging.info(e.args)
    maj=str(e.args)



try:
    url = "http://x.X.x.Z"
    response = urllib2.urlopen(url, timeout=5)
    r = response.read()
    lignes = r.splitlines()
    #chaine=lignes[3].split("|")
    temp=str('%(#).1f' % {'#': round(float(lignes[3]),1)})+''
    #temp="20.1C"
    logging.info(temp)
except urllib2.HTTPError, e:
    logging.info(e.code)
    maj=str(e.code)
except urllib2.URLError, e:
    logging.info(e.args)
    maj=str(e.args)


#try:
#    url = "http://x.X.x.Y"
#    response = urllib2.urlopen(url, timeout=5)
#    r = response.read()
#    lignes = r.splitlines()
#    chaine=lignes[3].split("|")
#    temp=str('%(#).1f' % {'#': round(float(chaine[1]),1)})+''
#    #temp="20.1C"
#    logging.info(temp)
#except urllib2.HTTPError, e:
#    logging.info(e.code)
#    maj=str(e.code)
#except urllib2.URLError, e:
#    logging.info(e.args)
#    maj=str(e.args)


try:
    url = "http://x.X.x.X/station_meteo.php"
    response = urllib2.urlopen(url, timeout=10)
    r = response.read()
    lignes = r.splitlines()
    chaine=lignes[0].split("|")
    temp1=str('%(#).1f' % {'#': round(float(chaine[0]),1)})+''
    logging.info(temp1)
    temp2=str('%(#).1f' % {'#': round(float(chaine[1]),1)})+''
    logging.info(temp2)
except urllib2.HTTPError, e:
    logging.info(e.code)
    maj=str(e.code)
except urllib2.URLError, e:
    logging.info(e.args)
    maj=str(e.args)


try:
    logging.info("station meteo")   
    epd = epd4in2.EPD()
    logging.info("init et clear")
    epd.init()
    epd.Clear()
    logging.info("definition fonts")
    font24 = ImageFont.truetype(os.path.join(fontsdir, 'Arial.TTF'), 24)
    font20 = ImageFont.truetype(os.path.join(fontsdir, 'arialbold.ttf'), 22)
    font18 = ImageFont.truetype(os.path.join(fontsdir, 'Arial.TTF'), 18)
    font19 = ImageFont.truetype(os.path.join(fontsdir, 'arialbold.ttf'), 19)
    font24b = ImageFont.truetype(os.path.join(fontsdir, 'arialbold.ttf'), 24)
    font16 = ImageFont.truetype(os.path.join(fontsdir, 'Arial.TTF'), 15)
    font14 = ImageFont.truetype(os.path.join(fontsdir, 'arialbold.ttf'), 14)
    font12 = ImageFont.truetype(os.path.join(fontsdir, 'Arial.TTF'), 12)
    font35b = ImageFont.truetype(os.path.join(fontsdir, 'arialbold.ttf'), 30)
    font35 = ImageFont.truetype(os.path.join(fontsdir, 'Arial.TTF'), 30)
    font40 = ImageFont.truetype(os.path.join(fontsdir, 'arialbold.ttf'), 35)
    font45b = ImageFont.truetype(os.path.join(fontsdir, 'arialbold.ttf'), 45)

    logging.info("creation image")
    Himage = Image.new('1', (epd.width, epd.height), 255)   # 255: clear the frame$
    draw = ImageDraw.Draw(Himage)

    logging.info("dessin des lignes")
    draw.line((130, 0, 130, 300), fill = 0)
    draw.line((131, 0, 131, 300), fill = 0)
    draw.line((132, 0, 132, 300), fill = 0)
    draw.line((220, 115, 220, 300), fill = 0)
    draw.line((221, 115, 221, 300), fill = 0)
    draw.line((222, 115, 222, 300), fill = 0)
    draw.line((310, 115, 310, 300), fill = 0)
    draw.line((311, 115, 311, 300), fill = 0)
    draw.line((312, 115, 312, 300), fill = 0)
    draw.line((130, 115, 400, 115), fill = 0)
    draw.line((130, 116, 400, 116), fill = 0)
    draw.line((130, 117, 400, 117), fill = 0)
    draw.text(( 155, 121), '+6H', font = font20, fill = 0)
    draw.text(( 240, 121), '+12H', font = font20, fill = 0)
    draw.text(( 330, 121), '+18H', font = font20, fill = 0)

    logging.info("dessins images statiques")
    bmp3 = Image.open(os.path.join(pic64, 'temp.bmp'))
    Himage.paste(bmp3, (0,153))
    #bmp4 = Image.open(os.path.join(pic64, 'humidity20.bmp'))
    #Himage.paste(bmp4, (0,280))
    bmp5 = Image.open(os.path.join(pic64, 'wind.bmp'))
    Himage.paste(bmp5, (0,258))
    bmp6 = Image.open(os.path.join(pic64, 'temp20.bmp'))
    Himage.paste(bmp6, (142,217))
    #bmp7 = Image.open(os.path.join(pic64, 'humidity20.bmp'))
    #Himage.paste(bmp7, (145,225))
    bmp8 = Image.open(os.path.join(pic64, 'wind20.bmp'))
    Himage.paste(bmp8, (142,267))
    bmp9 = Image.open(os.path.join(pic64, 'temp20.bmp'))
    Himage.paste(bmp9, (232,217))
    #bmp10 = Image.open(os.path.join(pic64, 'humidity20.bmp'))
    #Himage.paste(bmp10, (235,225))
    bmp11 = Image.open(os.path.join(pic64, 'wind20.bmp'))
    Himage.paste(bmp11, (232,267))
    bmp12 = Image.open(os.path.join(pic64, 'temp20.bmp'))
    Himage.paste(bmp12, (322,217))
    #bmp13 = Image.open(os.path.join(pic64, 'humidity20.bmp'))
    #Himage.paste(bmp13, (325,225))
    bmp14 = Image.open(os.path.join(pic64, 'wind20.bmp'))
    Himage.paste(bmp14, (322,267))


    logging.info("dessin dynamique des images en fx de la meteo")
    #image meteo guache
    bmp15 = Image.open(os.path.join(pic128, actu))
    Himage.paste(bmp15, (0,0))
    #image meteo +6
    bmp16 = Image.open(os.path.join(pic64, plus6))
    Himage.paste(bmp16, (145,143))
    #image meteo +12
    bmp17 = Image.open(os.path.join(pic64, plus12))
    Himage.paste(bmp17, (235,143))
    #image meteo +18
    bmp18 = Image.open(os.path.join(pic64, plus18))
    Himage.paste(bmp18, (325,143))


    logging.info("dessin text a partir des variables")
    #date-saint-lever-coucher-maj
    draw.text((int((110+((265-(len(date)*10))/2))), 0 ), date, font = font35b, fill = 0)
    draw.text((int((120+((265-(len(saint)*8))/2))), 31), saint, font = font24b, fill = 0)
    draw.text((int((125+((265-(len(ephemeride)*8))/2))), 60), ephemeride, font = font19, fill = 0)
    draw.text((312, 86), maj, font = font19, fill = 0)
    #draw.text(( 140, 65), press, font = font16, fill = 0)
    #colonne de gauche
    draw.text(( 26, 120), press, font = font19, fill = 0)
    draw.text(( 39, 141),  temp, font = font45b, fill = 0)
    draw.text(( 38, 183),  temp1, font = font40, fill = 0)
    draw.text(( 38, 216),  temp2, font = font40, fill = 0)
    draw.text(( 40, 251), vent, font = font24b, fill = 0)
    draw.text(( 40, 275), direction, font = font24b, fill = 0)
    #draw.text(( 40, 280), hum, font = font19, fill = 0)
    # prevision +6h
    draw.text(( 170, 217), temp6, font = font24b, fill = 0)
    #draw.text(( 170, 225), hum6, font = font19, fill = 0)
    draw.text(( 170, 252), vent6, font = font24b, fill = 0)
    draw.text(( 170, 275), direction6, font = font24b, fill = 0)
    #prevision +12h
    draw.text(( 260, 217), temp12, font = font24b, fill = 0)
    #draw.text(( 260, 225), hum12, font = font19, fill = 0)
    draw.text(( 260, 252), vent12, font = font24b, fill = 0)
    draw.text(( 260, 275), direction12, font = font24b, fill = 0)
    #prevision +24h
    draw.text(( 350, 217), temp18, font = font24b, fill = 0)
    #draw.text(( 350, 225), hum18, font = font19, fill = 0)
    draw.text(( 350, 252), vent18, font = font24b, fill = 0)
    draw.text(( 350, 275), direction18, font = font24b, fill = 0)




    epd.display(epd.getbuffer(Himage))
    epd.sleep()
    epd.Dev_exit()
    
except IOError as e:
    logging.info(e)
    
except KeyboardInterrupt:    
    logging.info("ctrl + c:")
    epd4in2.epdconfig.module_exit()
    exit()
