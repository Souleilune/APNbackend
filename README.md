Open this folder in Visual Studio or any code editor.

To run frontend:

1. Run command "cd APNTelemetry" first then run "npm install".
2. After installing the necessary packages go to .env file.
3. Run command in external command prompt then run "ipconfig".
4. Look for ipv4 address and save the ip address.
5. Go to the frontend folder and look for .env file.
6. Set the public expo file to your ip address.
7. If .env file is missing create one inside the frontend folder.
8. Copy and paste the values of API keys.
9. Make sure the expo public url is set in your own ip address.
10. Install Expo Go app in your mobile device.
11. Scan the qr code using the Expo Go app in your Android device. For IOS use the device scanner.

Troubleshoot:
If the project app is not loading in the Expo Go, ensure that you are using your ip address in any .env file.
If issues persist uninstall and install the app.

To run backend:
1. Run command "cd APNbackend" then run "npm install".
2. Create .env in the backend folder after packages are installed.
3. Use your ip address if its needed in the keys.
4. Copy and paste the values of API keys.
5. Run command "npm run dev"

Common problem:
App is running in mobile device but I can't login?
 > Ensure you are using the same ip address in frontend and backend, check the logs.
 > CTRL+C to stop both server.

