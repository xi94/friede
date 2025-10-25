# Friede - A thirdparty login manager for the Riot Client.
* It uses UI Automation to login efficiently, and finds all the installation directories on it's own.


# Preview
![qrFz7rg](https://github.com/user-attachments/assets/1258cd6b-e516-4cfe-b2cf-af8913976c36)


# Build Instructions
* This application uses my own build system [Talon](https://github.com/xi94/talon) . I don't recommend trying to build this yourself yet, as I still have to work on that project, but you can still feel free to attempt so.

* If you are going to attempt building it, make sure to install qt 6.9.3 to the default C:/ path, and add "C:\Qt\6.9.3\msvc2022_64\bin" to the environment path so that we can have access to moc and windeployqt.

* Then just run talon build -p release in the root directory to compile the application.
