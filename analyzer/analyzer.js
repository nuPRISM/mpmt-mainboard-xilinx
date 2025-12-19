var plot_update_timeout;
var plot_update_rate_ms;


var parse_rpc_response = function(rpc_result) {
    let status = rpc_result.status;
    let reply = "";
    
    if (status == 1) {
        // Only get a reply from mjsonrpc if status is 1                                                               
        let parsed = JSON.parse(rpc_result.reply);
        status = parsed["code"];
        reply = parsed["msg"];
    }
    
    return [status, reply];
};


var update_plots = function() {
    // Cancel any pending update
    clearTimeout(plot_update_timeout);
    
    // Update period for next time
    plot_update_rate_ms = 500;
    
    
    // Get the waveform data via JRPC
    let params = new Object;
    params.client_name = "mpmt_analyzer_py";
    params.cmd = "get_plot_data";
    params.args = JSON.stringify({"plot_names": ["plot1","plot2"]
				 });
    params.max_reply_length = 1024*1024;
    
    mjsonrpc_call("jrpc", params).then(function(rpc) {
	let [status, reply] = parse_rpc_response(rpc.result);
	
	if (status == 1) {
	    let jrep = JSON.parse(reply);


	    let div = document.getElementById("P1");
            let mpg = div.mpg;
	    
	    // Set the histogram to use this new data
	    mpg.setData(0,jrep["data"]);
	    plot_update_timeout = setTimeout(update_plots, plot_update_rate_ms);
	    

	} else {
	    alert_rpc_error(status, reply);
	}
	
    }).catch(function(error) {
	console.log(error);
	plot_update_timeout = setTimeout(update_plots, plot_update_rate_ms);
    });

};
    
    
    
var tinit = function() {
    // Get list of plot names and human-readable titles.                                                               
    plot_update_timeout = setTimeout(update_plots, 300);
}
