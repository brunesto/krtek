# krtek
Simple arduino project to help locate the origin of noises
It can handle either analog mics or digital probes.

# Menu description

## Detect

Displays the number of times a noise source has been identified.

The center of the scrren shows the result of last analysis
* ```-``` : sampled values too low
* ```!-``` : (in Digital mode only) not all probes detected a signal
* ```DT``` : difference in peak time is too long,  The peaks are likely to come from different noise sources.
* ```mX``` : the Xth (1,2,3) probe picked up the signal first

Buttons:
* UP: reset counters
* OK: return to the main menu

  
## Info


Displays the info pages (3 global pages plus 2 per mic).


### Info Modes
The pages have 3 modes: in NORMAL/TRIGGER/BLOCKED

in all modes, the button DOWN will cycle thru next page

in NORMAL mode:
* UP: enter trigger mode
* OK: return to the main menu


In TRIGGER mode, when one analysis succeeds, the mode becomes BLOCKED.
In TRIGGER mode, UP will return to NORMAL mode.


in BLOCKED mode, further recording/analysis are disabled
* The user can still navigate the info  pages with DOWN button.
* UP has no effect
* OK will return to NOIRMAL mode




### Info Pages


### Page 1
```p1/XASTS ```  page 1 of X, A:probe mode (Analog or Digital) , STS:status <br/>
```MV@TV    ```  maxv of mic1 was MV at sample TV<br/>
```MV@TV    ```  same for mic2<br/>
```         ``` <br/>

### Page 2
```p2/X     ```
```rt:XXX   ``` time spent sampling (inlcuding the us loop delay)  <br/>
```b:XXX    ``` periods, i.e. number of recording/analysis cycles performed  <br/>
```b/s:XXX  ``` periods per second <br/>

### Page 3
```p3/X     ```
```vs:      ``` samples recorded, one sample is for all mics  <br/>
```v/s:     ``` samples per seconds (i.e. sampling precision in Hz)  <br/>
```         ``` <br/>
### Page 4
```p4/X m1  ``` first page for mic 1  <br/>
```MV@TV    ``` maxv of mic1 was MV at sample TV  <br/>
```r:XXX-XXX``` minimum and maximum values sampled for mic 1  <br/>
```a:XXX d:X``` average and range <br/>

### Page 5
```p5/X m1  ``` 2nd page for mic 1 <br/>
```r:XXX    ``` the current raw value of the probe <br/>
```         ```  <br/>
```         ``` <br/>
### Further pages
2 further pages per additional mic are available


## Silence


Use this menu to automatically adjust MinV (See Config for the definition of MinV)

The sequence is as follows:
-1 second grace
-silent step: 5 second for detecting maximum sampled values (the environment should be silent)
-1 second grace
-noisy step: 5 second for detecting maximum sampled values (some small noise should be produced)

The the minV is computed as beeing the middle point between the maximums recorded during silent and noisy steps


  
## Config
enter  sub menu to config:

Loop us: specify microseconds delay after sampling all probes
Mics: number of probes
Max Dt: maximum difference in sampling sequence number between peak signals. When the peak signals are further apart than this number, the analysis fails as the peaks are likely to come from different noise sources.
Min V: Only used in analogue more. At least one of the probe must return maximum sampled value above Min.V  for the analysis to start.
Samples: Number of samples in a period
Digital: Toggle between Digital/Analog mode

  
## Oscilo

  record the values sampled and dump them on the screen. Note that there is only room for 60 bytes, so only the first 60/mics samples will be displayed
  OK: return to main menu
  UP: trigger mode on/off . In trigger mode when a noise source is identified the display will freeze, until OK is pressed and released
  
## About
Displays the version number  


  



## Metacfg

Sub menu for storing/reseting the config

Load: reload the configuration from EEPROM
Save: save the configuration to the EEPROM
Rst Ana: reset to factory settings for analogue probes (e.g microphone with 386 amplifier)
Rst Dgt: reset to factory settings for digital probes (e.g microphone with 363? comparator)
