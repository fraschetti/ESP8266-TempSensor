import java.io.IOException;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.apache.commons.lang3.StringUtils;
import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.handler.AbstractHandler;

public class OTAServer extends AbstractHandler {
	private static final Logger LOGGER = LogManager.getLogger(OTAServer.class.getName());
	
	public static void main(String[] args) throws Exception {
		LOGGER.log(Level.INFO, "Starting monitoring web app...");
		
		Server server = new Server(8080);
		server.setHandler(new OTAServer());

		server.start();
		server.join();
	}
	
	//Example filename - tempsensor_#.bin (where # is the new firmware version. ex: tempsensor_2.bin)

	public void handle(String target, Request baseRequest, HttpServletRequest request, HttpServletResponse response)
			throws IOException, ServletException {
		
		String hostname = request.getParameter("deviceid");
		if(StringUtils.isBlank(hostname))
			hostname = "Unknown";
		
		LOGGER.log(Level.INFO, "DeviceID: " + hostname);
		LOGGER.log(Level.INFO, "Request: " + target);
		LOGGER.log(Level.INFO, "URL Args: " + request.getQueryString());
		
		//quick hack 
		if(target.endsWith("tempsensor_invalid.bin"))
			target = "/ota/tempsensor.bin";
		
		String[] pathParts = StringUtils.split(target, "/");
		if(pathParts.length > 0) {		
			if(StringUtils.startsWithIgnoreCase(pathParts[0], "OTA")) {
				OTAHandler.handle(pathParts, baseRequest, request, response);
			}
		}
		
		if(!baseRequest.isHandled()) {
			response.setContentType("text/html;charset=utf-8");
			response.setStatus(HttpServletResponse.SC_NOT_FOUND);
			baseRequest.setHandled(true);
			response.getWriter().println("<h1>Invalid request</h1>");
		}
	}

}