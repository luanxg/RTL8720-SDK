example_aj_basic:

	* An example of Bus Method
	* Provided in the AllJoyn Thin Client source code pack 
	* Contains AllJoyn Basic Service and Client applications
	
	* Description:
		Service app provides Bus Method 'string cat(string s1, string s2)', which joins string 's1' and 's2' and returns the joined string 's1+s2'.
		Upon connected to router, Client app calls Service app's 'cat' method with arguments "Hello" and "World!", and Service app returns the joined string "HelloWorld!" to Client app.
	
	* Useage:
		compile client app : #define AJ_SERVICE_APP 0
		compile service app: #define AJ_SERVICE_APP 1
			
	* Tools:
	    TrustedTLSampleRN (under "\tools\alljoyn" directory)
	    
	* Possible use case:
			1) Start TrustedTLSampleRN
			2) Ameba1 runs Client app
			3) Ameba2 runs Service app
			4) Ameba1 and Ameba2 form communication through TrustedTLSampleRN
			
			
example_aj_ameba_led:

	* An example of Bus Property
	* Contains AllJoyn Property Client
	
	* Description:
		Mobile app runs Property Service, Ameba runs Property Consumer
		Property Server exposes Bus Property 'Val' to Consumer. Consumer calls getVal()/setVal() to obtain/update 'Val'
		At Consumer's side, 'Val' is changed by Push Button press. At Service's side, 'Val' is changed by UI Button click
		
	* Useage:
		extra components required: 
			3 LEDs and 3 Push Buttons
		circuit connection: 
			connect LED1 to PC_0
			connect LED2 to PC_2
			connect LED3 to PC_3
			connect PB1  to PC_1
			connect PB2  to PC_4
			connect PB3  to PC_5
			
	* Tools:
			AmebaLED.apk (under 'tools\alljoyn\alljoyn_ameba_led_example\Android' directory)
			
	* Possible use case (example video https://vimeo.com/163525162):
			1) Start mobile app
			2) Start Client 
			3) Press Push Buttons at Ameba side to see the mobile app's UI change 
			4) Press UI Buttons at mobile app side to see the LED's change

[Supported List]
	Source code not in project :
	    Ameba-1, Ameba-z, Ameba-pro