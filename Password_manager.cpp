#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <sys/stat.h>
using namespace std;


int getch() 
{
    /*
     * Found this function online. It allows me to take in a char and not put it out onto the screen
     */
    int ch;
    struct termios t_old, t_new;

    tcgetattr(STDIN_FILENO, &t_old);
    t_new = t_old;
    t_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
    return ch;
}

void clear_screen() 
{
    // Clear the screen so that passwords and other things aren't visible
    cout << string(200, '\n');
}

class loginManager 
{
    public:
        loginManager() 
        { 
            // Constructor that sets all initial values and sets the path for different files
            clear_screen();                     // clear the screen for use
            mkdir("Manager", 0777);             // create a directory to save every thing
            accessGranted = 0;
            logInID = -1;
            usernameFile = "Manager/users.dat"; // set the path for the two main files
            passwordFile = "Manager/pass.dat";
            createFile("users");                // create two files to save everything
            createFile("pass");
        }

        void login() 
        {	
            /* 
             * This is how we login to view all of the passwords
             */

            // Get the username
            cout << "\nPlease enter your username and password. \nUsername:";
            cin >> usernameAttempt;

            // Check the users file for the username
            int userID = checkFile(usernameFile, usernameAttempt);

            // input username has not been taken yet
            if (userID != 0) 
            {
                // Get the password attempt
                getch();
                passwordAttempt = getPass("Password:");

                // see if the password attempt is in the password file and if it corresponds to the correct username
                int pswrdID = checkFile(passwordFile, passwordAttempt, userID);
                if (userID == pswrdID) 
                {
                    // Now the user has logged in and we will save the ID
                    logInID = pswrdID;
                    accessGranted = true;
                    // Get the file name for that user's file
                    string file = "Manager/" + to_string(logInID) + ".dat";
                    userFile = file.c_str();
                    // run the log in function
                    loggedIn();
                    return;
                } 
                // wrong password
                cout << "Incorrect Password\n\n";
                return;
            }
            // no username
            cout << "Nice try bud\n\n";
        }

        bool addUser() 
        {
            /*
             * We add a user to the account and save their user/password
             */
            // check to see if the username is already existing (the ID should return 0 if not existing)
            string newUser, newPass, passCheck;
            bool samePass = false;
    		cout << "Please create a username:\n";
    		cin >> newUser;

            // make sure that it is a valid username
        	if(checkFile(usernameFile, newUser) != 0) 
            {
        		cout << "That username is not available\n";
        		return false;
        	}

            // clear getch() for the user input
            getch();

            // run this while the passwords are not the same
            while(!samePass) 
            {
                // Here we take in pass but replace all with *
                newPass = getPass("Please create a password:\n");
                passCheck = getPass("Please confirm the password:\n");
                // cin << passwordAttempt  <- Use this line instead of the following 8 if getch() isn't working
                if(passCheck == newPass)
                    samePass = true;
                else
                    cout << "Passwords do not match, please try again.\n";

            }
            // get the new user's ID and then add the username and password to respective files
        	logInID = 1 + getLastID(usernameFile);
        	saveFile(newUser, usernameFile, logInID);
        	saveFile(newPass, passwordFile, logInID);
            // Create a file for the new user
            string fileName = to_string(logInID);
            const char* newFile = fileName.c_str();
            userFile = createFile(newFile).c_str();
            accessGranted = true;
            loggedIn();
        	return true;
        }

        void logOut() 
        {
            // Log out
            accessGranted = false;
            logInID = -1;
            userFile = "";
            clear_screen();
            cout << "\nLOGGED OUT\n\n";
        }
        
        bool exists(string p_fileName) 
        {
            // determines if the file passed in exists or not
            ifstream file(p_fileName);
            return file.good();
        }


    private:
        string      usernameAttempt;    // last attempt by user to log in
        string      passwordAttempt;    // last attempt by user to log in
        const char* usernameFile;       // the file path for the usernames
        const char* passwordFile;       // file path for the passwords
        const char* userFile;           // once logged in, file path for the current user
        int         logInID;            // ID of the current user
        bool        accessGranted;      // keep track of logged in or not

        long long encrypt(int p_letter) { return powf(p_letter, 5) * 4 - 14; }          // choose whatever function you want here
        int decrypt(long long p_letter) { return powf((p_letter + 14) / 4, 1/5.f); }    // just make sure that you undo it here

        int checkFile(const char* p_fileName, string attempt, int p_id=-1) 
        {
            /*
             * Here is where we check the file and return the id of given attempt if the attempt is found
             */
            string line;
            ifstream file;

            string curr_char;
            long long eChar;

            file.open(p_fileName);

            while(1) 
            {
                // take in the first/next character
                file >> curr_char;
                // if our curr_char contains #ID:, we've just completed a username/password
                if(curr_char.find("#ID:") != string::npos) 
                {
                    // our attempt matches the lastest username/password
                    curr_char.erase(0,4);
                    int id;
                    istringstream(curr_char) >> id; 
                    // Users can have the same password, so we want to make sure we return the correct id, hence p_id
            		if(attempt == line && (id == p_id || p_id == -1)) 
                    {
            			file.close();
            			return id;
            		}
                    // our attempt is not the latest, so continue
            		line.erase(line.begin(), line.end());
                } 
                else 
                {
                    // convert the line into a long long and decrypt
                	istringstream(curr_char) >> eChar;
                	line += char(decrypt(eChar));
                }
                // we've reached EOF and not found our attempt 
                if(file.peek() == EOF) 
                {
                	file.close();
                	return 0;
                }
            }
        }

        void saveFile(string p_line, const char* p_fileName, const int& id) 
        {
            /*
             * We will save and update specified files 
             */
            // open the file, but only append
        	ofstream file(p_fileName, ios_base::app);

        	if(file.is_open()) 
            {
                // see if the file already has a username saved and add a line after the ID
                file.seekp(0, ios_base::end);
                if(file.tellp() != 0)
                    file << "\n";
                file.seekp(0, ios::beg);
                // encrypt and add our username/password char by char
        		for(int i=0; i < p_line.length(); i++)
                {
        			file << encrypt(p_line[i]);
        			file << "\n";
        		}
                // add the ID at the end
        		file << "#ID:" << id;
        		file.close();
        	} 
            else 
                cout << "No file to open\n";
        }

        int getLastID(const char* p_file) 
        {
            /*
             * Here is where we check the last ID to add a user to the file
             */

        	fstream file;
        	file.open(p_file, ios::in);
        	file.seekg(0, ios::end);
            // Check to see if we have an empty file 
        	if(file.tellg() == 0 || file.tellg() == -1)
        		return 0;

            // get the last character all the way up to the # sign
        	string s;
        	for(int i=-1; s.find('#') == string::npos; i--) 
            {
        		file.seekg(i, ios::end);
        		file >> s;
        	}

            // close the file the erase the #ID: from s
        	file.close();
        	s.erase(0, 4);
        	int id;
            // Turn s into an integer and return
        	istringstream(s) >> id;
        	return id;
        }

        void loggedIn() 
        {
            cout << "\nLOGGED IN\n\n";
            char option;
            // run loop while user is logged in
            while(accessGranted) 
            {
                cout << "Would you like to view info, add info, edit info, remove info, logout, or delete account? (V/A/E/R/L/D): ";
                cin >> option;
                option = tolower(option);
                clear_screen();
                if(option == 'v') 
                {
                    cout << endl;
                    printInfo();
                    cout << endl;
                } 
                else if(option == 'a') 
                    addPass();
                else if(option == 'e') 
                {
                    string service;
                    cout << "Please enter the service you would like to edit: ";
                    getline(cin.ignore(), service);
                    if(!changeInfo(service)) 
                        cout << "\nSorry, we couldn't find the service, please try again\n\n";
                    else
                        cout << "\nInfo changed successfully!\n\n";            
                } 
                else if(option == 'r')
                {
                    string service;
                    cout << "Please enter the service you would like to remove: ";
                    getline(cin.ignore(), service);
                    if(!removeInfo(service))
                        cout << "\nWe could not find the service you removed, please try again\n\n";
                    else
                        cout << "\nService removed succesfully!\n\n";                    
                }
                else if(option == 'l')
                    logOut();
                else if(option == 'd')
                {
                    cout << "Are you sure you want to delete your account? All data saved will be lost. (Y/N)\n";
                    cin >> option;
                    option = tolower(option);
                    if(option == 'y') {
                        deleteAccount(usernameFile);
                        string name = to_string(logInID);
                        name = "Manager/" + name + ".dat";
                        const char* file = name.c_str();
                        cout << "We've made it through deleteAccount()\n";
                        remove(file);
                        cout << "we've removed the file\n";
                        logOut();
                    }
                }
                else
                    cout << "Sorry there was an error, please try again.\n";
            }
        }
        
        void printInfo() 
        {
            string line;
            ifstream file;

            string curr_char;
            long long eChar;
            file.open(userFile);
            int count=0;

            while(1) 
            {
                // take in the first/next character
                file >> curr_char;
                // if our curr_char contains #ID:, we've just completed a username/password
                if(curr_char.find("#ID:") != string::npos) 
                {
                    // this means that we have reached the end of username, password, etc
                    if(count % 3 != 0)
                        cout << "   ";
                    cout << line << endl;
            		line.erase(line.begin(), line.end());
                    count = count + 1;
                } 
                else 
                {
                    // convert the line into a long long and decrypt
                	istringstream(curr_char) >> eChar;
                	line += char(decrypt(eChar));
                }
                // we've reached EOF and not found our attempt 
                if(file.peek() == EOF) 
                {
                    if(count == 0)
                        cout << "No info to display\n";
                    
                	file.close();
                	return;
                }
            }

        }

        void addPass() 
        {
            string service;
            string i_username;
            string i_password;
            cout << "Please enter the service for which username and password are being saved: ";
            getline(cin.ignore(), service);
            cout << "Please enter the username: ";
            getline(cin, i_username);
            cout << "Please enter the password: ";
            int id = getLastID(userFile);
            getline(cin, i_password);
            saveFile(service, userFile, id+1);
            saveFile(i_username, userFile, id+2);
            saveFile(i_password, userFile, id+3);
        }

        string getPass(string message) 
        {
            /*
             * this is how we put up '*' instead of actually showing the password
             */
            char ch;
            string password = "";
            cout << message;
            ch = getch();

            // run until the user presses ENTER
            while(ch != 10) 
            {
                // if the press BACKSPACE or DELETE
                if(ch == 8 || ch == 127) 
                {
                    // only delete what they have entered, nothing more
                    if(password.length() != 0) {
                        cout << "\b \b";
                        password.pop_back();
                    }
                } 
                else 
                {
                    cout << "*";
                    password += ch;
                }
                // get the next user's input
                ch = getch();
            }
            cout << endl;
            return password;
        }

        string createFile(const char* p_fileName) 
        {
            /*
             * Create a file given the name passed in 
             */
            string name = "Manager/" ;
            name.append(p_fileName);
            name.append(".dat");

            // check to see if it already exists so we don't write over it
            if(exists(name)) 
                return name;
            
            fstream file;
            file.open(name, ios::out);

            if(!file) 
            {
                cout << "Error in creating file!!!\n";
                return "Error";
            }

            // close file we've been working with and return the file path
            file.close();
            return name;
        }

        bool changeInfo(string service) 
        {
            /*
             * Here we can change the username/password associated with a given service
             */

            ifstream    file;                                       // the file we are accessing (userFile)
            string      line;                                       // this is how we store service, username, password 
            string      curr_char;                                  // the current line we are on in the file (which is a char in the word)
            string      message[2] = {"Username", "Password"};      // message to ask user for username or password
            long long   eChar;                                      // store the curr_char and use to decrypt to add to line
            int         count = 0;                                  // this is how we will keep track of our ID's
            char        option;                                     // the user's input
            bool        found = false;                              // Way to see if we have found the service user wanted to change

            // create a temporary file to be used
            const char* temp = createFile("temp").c_str();
            file.open(userFile);

            while(1) 
            {
                // take in the first/next character
                file >> curr_char;
                // if our curr_char contains #ID:, we've just completed a service/username/password
                if(curr_char.find("#ID:") != string::npos) 
                {
                    // our attempt matches the lastest username/password
            		if(service == line && count % 3 == 0) 
                    {
                        found = true;
                        for(int i=0; i < 2; i++) 
                        {
                            // Add the service line to our temp file
            			    saveFile(line, temp, count);
                            line.erase(line.begin(), line.end());
                            // check to see if they want to change username
                            cout << "Change " << message[i] << "? (Y/N) ";
                            cin >> option;
                            option = tolower(option);

                            if(option == 'y') 
                            {
                                // get the new username and save it to the line
                                cout << "Please enter the new " << message[i] << ": ";
                                getline(cin.ignore(), line);
                                // skip over the old username bc we don't need it anymore
                                file >> curr_char;
                                while(curr_char.find("#ID:") == string::npos) 
                                    file >> curr_char;
                            } 
                            else 
                            {
                                // Otherwise we keep the old username and store in line
                                file >> curr_char;
                                while(curr_char.find("#ID:") == string::npos) 
                                {
                                    istringstream(curr_char) >> eChar;
                                    line += char(decrypt(eChar));
                                    file >> curr_char;
                                }

                            }
            		    } 
                    }  
                    // update the temp file with whatever we have saved
                    saveFile(line, temp, count);
                    count += 1;
                    // erase the line to start again 
            		line.erase(line.begin(), line.end());
                } 
                else 
                {
                    // convert the line into a long long and decrypt
                	istringstream(curr_char) >> eChar;
                	line += char(decrypt(eChar));
                }
                // we've reached EOF and not found our attempt 
                if(file.peek() == EOF) 
                {
                	file.close();
                	break;
                }
            }
            // remove the od file and then rename the temporary file
            remove(userFile);
            rename(temp, userFile);
            return found;
        }

        bool removeInfo(string service) 
        {
            /*
             * This function is used to remove a service and its username and password from our database
             */

            ifstream    file;           // the file we are accessing (userFile)
            string      line;           // this is how we store service, username, password 
            string      curr_char;      // the current line we are on in the file (which is a char in the word)
            long long   eChar;          // store the curr_char and use to decrypt to add to line
            int         count = 0;      // this is how we will keep track of our ID's
            bool        found = false;  // way to tell if we found service user wanted to remove

            // get the file path for temp file
            const char* temp = createFile("temp").c_str();
            file.open(userFile);
            while(1) 
            {
                // take in the first/next char
                file >> curr_char;
                // if our curr_char contains "#ID:" then check it against our service
                if(curr_char.find("#ID:") != string::npos) 
                {
                    // we've found the service
                    if(line == service) 
                    {
                        found = true;
                        line.erase(line.begin(), line.end());
                        for(int i=0; i < 2; i++) 
                        {
                            // skip through the username and password associated with the service
                            file >> curr_char;
                            while(curr_char.find("#ID:") == string::npos) 
                                file >> curr_char;
                        }
                    } 
                    else 
                    {
                        // update the temporary file with all other lines 
                        saveFile(line, temp, count);
                        count += 1;
                        line.erase(line.begin(), line.end());
                    }
                }
                else 
                {
                    // convert the line into a long long and decrypt
                	istringstream(curr_char) >> eChar;
                	line += char(decrypt(eChar));
                }
                // we've reached EOF and not found our attempt 
                if(file.peek() == EOF) 
                {
                	file.close();
                	break;
                }
            }
            // remove the od file and then rename the temporary file
            remove(userFile);
            rename(temp, userFile);
            return found;
        }

        void deleteAccount(const char* p_filename) 
        {
            ifstream file;
            string line;
            string curr_char;
            long long eChar;
            int id = 1;
            int currID;

            // get the file path for the temp file
            const char* temp = createFile("temp").c_str();
            cout << "Opening " << p_filename << endl;
            file.open(p_filename);
            while(1) 
            {
                // here is where we will remove the user/pass
                file >> curr_char;
                if(curr_char.find("#ID:") != string::npos) 
                {
                    curr_char.erase(0,4);
                    istringstream(curr_char) >> currID;   
                    if(currID != logInID)
                        saveFile(line, temp, id);
                    id += 1;
                    line.erase(line.begin(), line.end());
                }
                else 
                {
                    // convert the line into a long long and decrypt
                    istringstream(curr_char) >> eChar;
                    line += char(decrypt(eChar));
                }
                // we've reached EOF and not found our attempt 
                if(file.peek() == EOF) 
                {
                    cout << "closing out of " << p_filename << endl;
                    file.close();
                    break;
                }
            }
            // remove old file and rename temp
            remove(p_filename);
            rename(temp, p_filename);
            // recursively call on using the password file
            if(p_filename != passwordFile)
                deleteAccount(passwordFile);
            return;
        }
};

int main() {
    loginManager app;
    char option;
    while(1) {
    	cout << "New or existing user?(n/e/q)\n";
    	cin >> option;
    	if(tolower(option) == 'n') {
    		app.addUser();
    	} else if(tolower(option) == 'e') {
    		app.login();
    	} else if(tolower(option) == 'q') {
    		cout << "bye.\n";
    		return 0;
    	}
    }
}
