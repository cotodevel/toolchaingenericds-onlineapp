![ToolchainGenericDS](img/TGDS-Logo.png)

This is the ToolchainGenericDS Woopsi OnlineApp project:

1.	Compile Toolchain:
To compile this project you will need to follow the steps at https://bitbucket.org/Coto88/toolchaingenericds :
Then simply extract the project somewhere.

2.	Compile this project: 
Open msys, through msys commands head to the directory your extracted this project.
Then write:
make clean <enter>
make <enter>

After compiling, run the example in NDS. 

Project Specific description:
A Woopsi UI + TGDS SDK project. 

Online TGDS Catalog. Lists and downloads TGDS content!

Shop Theme: 
Dan Truman - Instrumental Piano Music
Chill Instrumental Music [Short] FEEL GOOD JAZZ

Copy /release/arm7dldi-ntr folder contents in SD root. Enjoy the app!

History:
20/February/2021: 0.1 Alpha release


Known issues:
Background music while downloading a file will break connection. This is a bug in the DSWIFI code as interrupts seem to get in the way of wifi receive buffers.

Coto