#include <iostream>
#include <boost/program_options.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>


#include "shinysocks.h"


using namespace std;
using namespace shinysocks;
using boost::asio::ip::tcp;

void SleepUntilDoomdsay()
{
    boost::asio::io_service main_thread_service;

    boost::asio::signal_set signals(main_thread_service, SIGINT, SIGTERM
#ifdef SIGQUIT
        ,SIGQUIT
#endif
        );
    signals.async_wait([](boost::system::error_code /*ec*/, int signo) {

        BOOST_LOG_TRIVIAL(info) << "Reiceived signal " << signo << ". Shutting down";
    });

    BOOST_LOG_TRIVIAL(debug) << "Main thread going to sleep - waiting for shtudown signal";
    main_thread_service.run();
    BOOST_LOG_TRIVIAL(debug) << "Main thread is awake";
}


char *host_interface = "0.0.0.0";
char *local_port = "1080";


int main(int argc, char **argv) {

	

	if (argc > 1) {
		host_interface = argv[1];
		local_port = argv[2];
	}
    namespace po = boost::program_options;
    
    bool run_as_daemon = false;

	
	
    
    vector<unique_ptr<Listener>> listeners;
 

    Manager::Conf conf;
 
    conf.io_threads =4;
    

    // Start acceptor(s)
    Manager manager(conf);
    {
        boost::asio::io_service io_service;
         
		auto host = host_interface; //  node.second.get<string>("hostname");
		auto port = local_port; // node.second.get<string>("port");

        BOOST_LOG_TRIVIAL(error) << "Resolving host=" << host << ", port=" << port;

        tcp::resolver resolver(io_service);
        auto address_it = resolver.resolve({host, port});
        decltype(address_it) addr_end;

        for(; address_it != addr_end; ++address_it) {
            auto iface = make_unique<Listener>(manager, *address_it);
            iface->StartAccepting();
            listeners.push_back(move(iface));
        }
         
    }

#ifndef WIN32
    if (run_as_daemon) {
        BOOST_LOG_TRIVIAL(info) << "Switching to system daemon mode";
        daemon(1, 0);
    }
#endif


    // Wait for signal
    SleepUntilDoomdsay();

    manager.Shutdown();

    // Just wait for the IO thread(s) to end.
    manager.WaitForAllThreads();

    return 0;
}
