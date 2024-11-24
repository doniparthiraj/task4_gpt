# ChatBot

installations
    - sudo apt-get install uuid-dev

for server
    -gcc -o server server.c -luuid

for client
    -gcc -o client client.c

format:

/active
/send id message
/logout
/chatbot login
/chatbot logout
/history id
/history_delete id
/delete_all
/chatbot_v2 login
/chatbot_v2 logout
