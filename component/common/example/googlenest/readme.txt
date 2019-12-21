##################################################################################
#                                                                                #
#                             Example Googlenest                                 #
#                                                                                #
##################################################################################

Date: 2018-06-04

Table of Contents
~~~~~~~~~~~~~~~~~
 - Description
 - Setup Guide
 - Result description
 - Parameter Setting and Configuration
 - Other Reference
 - Supported List

 
Description
~~~~~~~~~~~
        Three examples are provided:
            The first example shows how a device put state to database every one seconds. 
            The second example shows how to get data from database. 
            The third example has two threads to show the flow both get data and put data through database.
	
Setup Guide
~~~~~~~~~~~
	Example1: Storing data from device
                Activate browser and paste the url of the Firebase address in the browser, then it is able to
                check the status changed overtime.
                Making some modifies:
                a.)In platform_opts.h, define CONFIG_EXAMPLE_GOOGLE_NEST to 1
                   #define CONFIG_EXAMPLE_GOOGLE_NEST 1
                b.)In example_entry.c, choose the type ¡§#define TYPE FromDevice¡¨
                 ¡§FromDevice¡¨ shows how data is transferred from device (Ameba) to database.
                c.)Define your Firebase Account address in ¡§example_google.h¡¨
                   # define HOST_ADDR xxxxxx
                d.)Build project and download it into Ameba
        Example2: Reading data
                In this example, the RGB information of a light will be controlled by a webpage through Firebase.
                Note: The webpage cannot access the Firebase for the Internet of Mainland of China. So ameba
                may show that the data is ¡§null¡¨.
                Making some modifies : Editing the Firebase address in the example.html.
                a.)In platform_opts.h, define CONFIG_EXAMPLE_GOOGLE_NEST to 1
                   #define CONFIG_EXAMPLE_GOOGLE_NEST 1
                b.)In example_entry.c, choose the type ¡§#define TYPE ToDevice¡¨
                 ¡§ToDevice¡¨ example shows how user update data from remote controller (using
                   javascript) to device (Ameba) via database.
                c.)Define your Firebase Account address in ¡§example_google.h¡¨ and the html code
                   # define HOST_ADDR xxxxxx
                d.)Inserting data to your Firebase using the webpage provided. Then building this code and the
                   data will be retrieved from Firebase. You can control the value of RGB through webpage at any
                   time to test retrieving the latest data.
        Example3: Reading and writing data at the same time
                In this example, there are two threads running at the same time. One thread is running to
                update the data of ¡§Motion Sensor¡¨ and ¡§light¡¨, and the other thread is getting the latest value
                of ¡§light¡¨ from Firebase.
                {
                   ¡§Motion_Sensor¡¨ : ¡§i¡¨,
                   ¡§Light¡¨ : {
                   ¡§Red¡¨ : ¡§0¡¨,
                   ¡§Green¡¨ : ¡§0¡¨,
                   ¡§Blue¡¨ : ¡§0¡¨,
                    }
                }
                The value of Motion_Sensor and Light will be changed every 5 seconds.

                Activate browser and paste the url of the Firebase address in the browser, then it is able to
                check the status changed overtime.
                Making some modifies:
                a.)In platform_opts.h, define CONFIG_EXAMPLE_GOOGLE_NEST to 1
                   #define CONFIG_EXAMPLE_GOOGLE_NEST 1
                b.)In example_entry.c, choose the type ¡§#define TYPE BOTH¡¨
                  ¡§BOTH¡¨ shows how data is read and wrote by device (Ameba) to database.
                c.)In FreeRTOSConfig.h, increasing the value of configTOTAL_HEAP_SIZE to make sure the
                   two threads have enough heap size to run.
                   #define configTOTAL_HEAP_SIZE ( ( size_t ) ( 110 * 1024 ) ) // use HEAP5
                d.)Define your Firebase Account address in ¡§example_google.h¡¨
                   # define HOST_ADDR xxxxxx
                e.)Build project and download it into Ameba

	
Result description
~~~~~~~~~~~~~~~~~~
        Example1: Shows how to update data to Firebase.
        Example2: Shows how to get the latest data from Firebase.
        Example3: Shows how to update data to Firebase and get the latest data from Firebase.


Parameter Setting and Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Please refer to AN0038 Realtek googlenest user guide.pdf
	

Other Reference
~~~~~~~~~~~~~~~
        For more details, please refer to AN0038 Realtek googlenest user guide.pdf

Supported List
~~~~~~~~~~~~~~
[Supported List]
        Supported : 
           Ameba-1, Ameba-z