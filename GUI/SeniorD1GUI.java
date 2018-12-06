import javax.swing.*;
import javax.imageio.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.image.*;
import java.util.*;
import java.io.*;
import java.text.SimpleDateFormat;
import javax.swing.event.*;
import java.awt.Graphics.*;
import java.lang.*;
import javax.swing.filechooser.FileSystemView;
import java.io.File;
//import java.awt.Dialog.ModalityType;

public class SeniorD1GUI{
	private static JFrame frame;
	private static JLabel distance;
	private static JLabel bottomText2;
	private static JSplitPane plotPanel;
	private static JPanel filesPanel;
	private static JPanel graphPanel;
	private static double speedOfSound = 5400.0;
	private static double calibrationSpeed = -1;
	private static double nCal = -1;
	
	private static final String mainPanelID = "mainPanel";
	private static final String plotPanelID = "plotPanel";
	private static final String fileSuffix = ".ndt";
	
	private static boolean alreadyOpenedPlot = false;
	
	public static void main(String args[]){
		frame = new JFrame("Senior Design One - Ultrasonic Flaw Detection");
		
		CardLayout cardLayout = new CardLayout();
		JPanel cards = new JPanel(cardLayout);
		JPanel mainPanel = new JPanel(new GridBagLayout());
		cards.add(mainPanel, mainPanelID);
		cardLayout.show(cards, mainPanelID);
		
		Font generalFont = new Font("Copperplate Gothic", Font.PLAIN, 25);
		Font boldFont = new Font("Copperplate Gothic", Font.BOLD, 25);
		
		GridBagConstraints gbc = new GridBagConstraints();
		gbc.fill = GridBagConstraints.BOTH;
		gbc.weightx = 1;
		gbc.weighty = 0;
		gbc.gridheight = 1;
		gbc.gridwidth = 3;
		gbc.gridx = 0;
		gbc.gridy = 0;
		
		
		Color osuOrange = new Color(255, 106, 20);
		BufferedImage myPicture = null;
		try{
			myPicture = ImageIO.read(new File("starwars_title.png"));
		} catch (Exception e){
			//Do nothing b/c I don't care.
		}
		JLabel picLabel = new JLabel(new ImageIcon(myPicture));
		JPanel titlePanel = new JPanel();
		titlePanel.setBackground(osuOrange);
		titlePanel.add(picLabel);
		mainPanel.add(titlePanel, gbc);
		
		JButton data = new JButton("Data & Graph");
		data.setFont(generalFont);
		gbc.weighty = 1;
		gbc.gridwidth = 1;
		gbc.gridy = 1;
		mainPanel.add(data, gbc);
		
		JButton measure = new JButton("Measure");
		measure.addActionListener(e -> doMeasureAndShow());
		measure.setFont(generalFont);
		gbc.gridx = 1;
		mainPanel.add(measure, gbc);
		
		JButton calibrate = new JButton("Calibrate");
		calibrate.addActionListener(e -> doCalibrate());
		calibrate.setFont(generalFont);
		gbc.gridx = 2;
		mainPanel.add(calibrate, gbc);
		
		JPanel distancePanel = new JPanel(new GridBagLayout());
		gbc.gridx = 0;
		gbc.gridy = 2;
		gbc.weighty = 0;
		gbc.gridwidth = 3;
		mainPanel.add(distancePanel, gbc);
		
		JLabel bottomText = new JLabel("Distance to flaw = ");
		bottomText.setFont(generalFont);
		distance = new JLabel("12");
		distance.setFont(boldFont);
		bottomText2 = new JLabel(" cm (5400 m/s)");
		bottomText2.setFont(generalFont);
		JLabel filler1 = new JLabel();
		JLabel filler2 = new JLabel();
		
		gbc.gridx = 0;
		gbc.gridy = 0;
		gbc.weightx = 1;
		gbc.gridwidth = 1;
		distancePanel.add(filler1, gbc);
		gbc.gridx = 1;
		gbc.weightx = 0;
		distancePanel.add(bottomText, gbc);
		gbc.gridx = 2;
		distancePanel.add(distance, gbc);
		gbc.gridx = 3;
		distancePanel.add(bottomText2, gbc);
		gbc.gridx = 4;
		gbc.weightx = 1;
		distancePanel.add(filler2,gbc);
		
		//Make plotting panel stuff
		filesPanel = new JPanel(new BorderLayout());
		graphPanel = new JPanel(new GridBagLayout());
		plotPanel = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, filesPanel, graphPanel);
		
		gbc.weighty = 0;
		gbc.gridy = 2;
		JButton returnButton = new JButton("Return to main menu");
		returnButton.addActionListener(e -> cardLayout.show(cards, mainPanelID));
		filesPanel.add(returnButton, BorderLayout.SOUTH);
		
		data.addActionListener(e -> {
			if(alreadyOpenedPlot) filesPanel.remove(((BorderLayout)filesPanel.getLayout()).getLayoutComponent(BorderLayout.CENTER));
			alreadyOpenedPlot = true;
			File dir = new File(".");
			File[] filesList = dir.listFiles();
			ArrayList<String> filesToDisp = new ArrayList<String>();
			for (File file : filesList) {
				if (file.isFile() && file.getName().toLowerCase().endsWith(fileSuffix)) {
					filesToDisp.add(file.getName());
				}
			}
			
			JList<String> fileList = new JList<String>(filesToDisp.toArray(new String[filesToDisp.size()]));
			fileList.setSelectionMode(DefaultListSelectionModel.SINGLE_SELECTION);
			
			MouseListener mouseListener = new MouseAdapter() {
				public void mouseClicked(MouseEvent e) {
				   String selectedItem = fileList.getSelectedValue();
				   doPlot(selectedItem);
				}
			};
			fileList.addMouseListener(mouseListener);
			
			fileList.setMinimumSize(returnButton.getMinimumSize());
			fileList.setPreferredSize(returnButton.getMinimumSize());
			
			filesPanel.add(fileList, BorderLayout.CENTER);
			
			cardLayout.show(cards, plotPanelID);
		});
		
		cards.add(plotPanel, plotPanelID);
				
		frame.add(cards);
		frame.setSize(1000,600);
		frame.setLocationRelativeTo(null);
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		frame.setVisible(true);
	}
	
	private static void doPlot(String filename){
		System.out.println(filename);
	}
	
	private static void updateSpeed(double newSpeed){
		speedOfSound = newSpeed;
		bottomText2.setText(" cm (" + Double.toString(speedOfSound) + " m/s)");
	}
	
	private static double doMeasure(){
		String timeStamp = new SimpleDateFormat("yyyyMMdd_HHmmss").format(Calendar.getInstance().getTime());
		String fileName = timeStamp + fileSuffix;
		return doMeasure(fileName);
	}
	
	private static double doMeasure(String s){
		String fileName = s;
		Runtime rt = Runtime.getRuntime();
		double distanceToFlaw = 0;
		try{
			Process pr = rt.exec("sudo chrt -f 90 taskset -c 2 ./register_level_gpio.c " + fileName);
			pr.waitFor();
			BufferedReader reader = new BufferedReader(new FileReader(fileName));
			String line = reader.readLine();
			while(line != null){
				if(Float.parseFloat(line.split(",")[0]) > 0.3){
					distanceToFlaw = Double.parseDouble(line.split(",")[1]) / 2 * 0.000001 * speedOfSound;
					break;
				}
			}
		} catch(Exception e){
			//Who cares?
		}
		return distanceToFlaw;
	}
	
	private static void doMeasureAndShow(){
		distance.setText(Double.toString(doMeasure()));
	}
	
	private static void setCalSpeed(double distanceToFlaw){
		String fileName = "temp";
		Runtime rt = Runtime.getRuntime();
		//double distanceToFlaw = 0;
		try{
			Process pr = rt.exec("sudo chrt -f 90 taskset -c 2 ./register_level_gpio.c " + fileName);
			pr.waitFor();
			BufferedReader reader = new BufferedReader(new FileReader(fileName));
			String line = reader.readLine();
			while(line != null){
				if(Float.parseFloat(line.split(",")[0]) > 0.3){
					double tempSpeed = distanceToFlaw*2/(Double.parseDouble(line.split(",")[1])*0.000001);
					if (nCal == -1) {
						calibrationSpeed = tempSpeed;
						nCal = 1;
					} else if (nCal > 0){
						nCal++;
						calibrationSpeed = (nCal-1)*calibrationSpeed/nCal + tempSpeed/nCal;
					} 
					return;
				}
			}
		} catch(Exception e){
			//Who cares?
		}
	}
	
	private static void doCalibrate(){
		final String calpan = "calpan";
		final String confirmpan = "confirmpan";
		JDialog calibrationDialog = new JDialog(frame, "Calibration - Please enter distance to flaw");
		JPanel topLevelCalPanel = new JPanel(new CardLayout());
		JPanel calibrationPanel = new JPanel(new GridBagLayout());
		topLevelCalPanel.add(calibrationPanel, calpan);
		GridBagConstraints gbc = new GridBagConstraints();
		gbc.fill = GridBagConstraints.BOTH;
		gbc.weightx = 1;
		gbc.weighty = 0;
		gbc.gridheight = 1;
		gbc.gridwidth = 1;
		gbc.gridx = 0;
		gbc.gridy = 0;
		JTextField distanceField = new JTextField();
		calibrationPanel.add(distanceField, gbc);
		JButton readyButton = new JButton("   Ready   ");
		gbc.gridx = 1;
		gbc.weightx = 0.1;
		calibrationPanel.add(readyButton, gbc);
		
		gbc.fill = GridBagConstraints.NONE;
		gbc.gridx = 0;
		gbc.gridy = 0;
		gbc.weightx = 1;
		gbc.gridwidth = 2;
		JPanel confirmPanel = new JPanel(new GridBagLayout());
		JLabel confirmLabel = new JLabel();
		confirmPanel.add(confirmLabel, gbc);
		gbc.fill = GridBagConstraints.BOTH;
		gbc.gridy = 1;
		gbc.gridwidth = 1;
		JButton weGood = new JButton("Accept Value");
		confirmPanel.add(weGood, gbc);
		gbc.gridx = 1;
		JButton nyb = new JButton("Continue Calibration");
		confirmPanel.add(nyb, gbc);
		topLevelCalPanel.add(confirmPanel, confirmpan);
		((CardLayout) (topLevelCalPanel.getLayout())).show(topLevelCalPanel, calpan);
		//calibrationDialog.setModalityExclusionType(Dialog.ModalExclusionType.APPLICATION_EXCLUDED);
		calibrationDialog.setModal(true);
		calibrationDialog.add(topLevelCalPanel);
		calibrationDialog.pack();
		calibrationDialog.setSize(300,70);
		calibrationDialog.setLocationRelativeTo(null);
		
		
		readyButton.addActionListener(e -> {
			double distanceVal = 0;
			try{
				distanceVal = Double.parseDouble(distanceField.getText());
			} catch (Exception ex){
				System.out.println("Unable to parse: \"" + distanceField.getText() + "\"" );
				return;
			}
			setCalSpeed(distanceVal);
			confirmLabel.setText("Calibrated speed: " + Double.toString(calibrationSpeed) + " m/s");
			((CardLayout) (topLevelCalPanel.getLayout())).show(topLevelCalPanel, confirmpan);
			calibrationDialog.setSize(300,85);
		});
		
		nyb.addActionListener(e -> {
			((CardLayout) (topLevelCalPanel.getLayout())).show(topLevelCalPanel, calpan);
			calibrationDialog.setSize(300,70);
		});
		
		weGood.addActionListener(e -> {
			updateSpeed(calibrationSpeed);
			nCal = -1;
			calibrationDialog.dispose();
		});
		
		calibrationDialog.setVisible(true);
	}
}


		
		