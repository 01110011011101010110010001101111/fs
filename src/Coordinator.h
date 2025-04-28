class Coordinator {
    public:
        // clients request a file from the coordinator. If it doesn't exist, hash the filename and put it into the corresponding server 
        int requestFile(std::string filename);
        std::map<std::string, int> file_server_data;
    
    private:
        // Map of [file] -> server
        
}