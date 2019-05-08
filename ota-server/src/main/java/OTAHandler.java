import java.io.File;
import java.io.FileInputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Arrays;
import java.util.Comparator;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.apache.commons.io.IOUtils;
import org.apache.commons.lang3.StringUtils;
import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.eclipse.jetty.server.Request;

public class OTAHandler {
	private static final Logger LOGGER = LogManager.getLogger(OTAHandler.class.getName());
	
	public static void handle(String[] pathParts, Request baseRequest, HttpServletRequest request, HttpServletResponse response)
			throws IOException, ServletException {
		
		//TODO add graceful shutdown method
		
		//pathParts[0] = OTA
		//pathParts[1] = requested file
		
		String httpFieldsStr = baseRequest.getHttpFields().toString().trim();
		httpFieldsStr = "    " + StringUtils.join(StringUtils.split(httpFieldsStr, "\r\n"), "\n    ");
		
		LOGGER.log(Level.INFO, "Request headers:\n" + httpFieldsStr);
		
		if(pathParts.length < 2)
			return;
		
		if(!StringUtils.endsWithIgnoreCase(pathParts[1], ".bin"))
			return;
		
		String otaFile = pathParts[1].substring(0, StringUtils.lastIndexOfIgnoreCase(pathParts[1], ".bin"));
		if(StringUtils.isBlank(otaFile))
			return;
		
		File otaDir = new File("c:\\temp\\ota\\");
		File[] otaFiles = otaDir.listFiles(new FilenameFilter() {
			
			@Override
			public boolean accept(File dir, String name) {
				return StringUtils.startsWithIgnoreCase(name, otaFile)
						&& StringUtils.endsWithIgnoreCase(name, ".bin")
						&& getBinVersion(name) != null;
			}
		});
		
		if(otaFiles.length == 0)
			return;
		
		//Reverse sort the list
		Arrays.sort(otaFiles, new Comparator<File>() {
		    public int compare(File f1, File f2) {
		    	Integer f1_ver = getBinVersion(f1.getName());
		    	Integer f2_ver = getBinVersion(f2.getName());
		    	
		        return f2_ver.compareTo(f1_ver);
		    }
		});
		
		File latestBinFile = otaFiles[0];
		String latestBinName = latestBinFile.getName();
		
		//We should never see a null here as we've already called this function
		//twice on this string
		Integer latestBinVer = getBinVersion(latestBinName);
		
		String espBinVersionStr = request.getHeader("x-ESP8266-version");
		if(StringUtils.isBlank(espBinVersionStr))
			espBinVersionStr = "-1";
		
		int espBinVer = Integer.parseInt(espBinVersionStr);
		
		if(latestBinVer > espBinVer) {
			LOGGER.log(Level.INFO, "Returning new firmware: " + latestBinName);
			writeFileToResponse(latestBinFile.getAbsolutePath(), baseRequest, response);
		} else {
			LOGGER.log(Level.INFO, "Device is already at latest firmware version");
			response.setContentType("text/html;charset=utf-8");
			response.setStatus(HttpServletResponse.SC_NOT_MODIFIED);
			baseRequest.setHandled(true);
			response.getWriter().println("<h1>Unchanged</h1>");
		}
	}
	
	private static Integer getBinVersion(String filename) {
		try {
			String binVerStr = filename.substring(
					StringUtils.lastIndexOf(filename, "_") + 1,
					StringUtils.lastIndexOfIgnoreCase(filename, ".bin"));
			
			return Integer.parseInt(binVerStr);
		} catch (Exception ex) {
			LOGGER.log(Level.ERROR, "Failed to parse bin version from: " + filename);
			return null;
		}
	}
	
	@SuppressWarnings("deprecation")
	private static void writeFileToResponse(String filePath, Request baseRequest, HttpServletResponse response) throws IOException {
		FileInputStream fis = null;
		OutputStream out = null;

        try {
        	File fileInfo = new File(filePath);        	
        	
            fis = new FileInputStream(filePath);
            response.setContentType("application/octet-stream");
            response.setContentLengthLong(fileInfo.length());
            response.setStatus(HttpServletResponse.SC_OK);

            out = response.getOutputStream();
            IOUtils.copy(fis, out); // this is using apache-commons, 
                                    // make sure you provide required JARs
            
            baseRequest.setHandled(true);
        } finally {            
            IOUtils.closeQuietly(out);  // this is using apache-commons, 
            IOUtils.closeQuietly(fis);  // make sure you provide required JARs
        }
	}
}
