import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.io.*;
import javax.swing.*;
import javax.swing.event.*;
import java.awt.Graphics.*;
import java.lang.*;

public class SeniorD1GUI{
	private static int x = 0;
	private static int y = 0;
	private static boolean pressed = false;
	private static boolean check = false;
	private static int tempX, tempY;
	private static GUIFrame object = new GUIFrame();
	private static long count;
	private static int jpg = 0;
	/*
    public void paintComponent(Graphics g) {
        super.paintComponent(g);
        ImageIcon i = new ImageIcon("GUI.png");
		if (jpg == 0) i = new ImageIcon("GUI.jpg");
		else if (jpg == 1) i = new ImageIcon("GUI_DG_pressed.jpg");
		else if (jpg == 2) i = new ImageIcon("GUI_M_pressed.jpg");
		else if (jpg == 3) i = new ImageIcon("GUI_S_pressed.jpg");
        i.paintIcon(this, g, 0, 0);
    }
	*/
	
    public static void main(String[] args){
        //SeniorD1GUI inter = new SeniorD1GUI();
        JFrame jf = new JFrame();
        jf.setTitle("Senior Design GUI");
        jf.setSize(1295,760);
        jf.setVisible(true);
        jf.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        jf.add(object);
		object.addMouseListener(new AL());
    }
	
	static class AL extends MouseAdapter{
		public void mousePressed (MouseEvent e){
			x = e.getX();
			y = e.getY();
			tempX = x;
			tempY = y;
			if (!check){
				if (y >= 196 && y < 614){
					if (x >= 0 && x < 430) jpg = 1;
					else if (x >= 430 && x < 852) jpg = 2;
					else if (x >= 852 && x < 1290) jpg = 3;
				}
			}
			else {
				if (y >= 16 && y <= 73 && x >= 18 && x <= 113) {
					pressed = true;
				}
			}
			object.framing(x, y, jpg, pressed);
			x = 0;
			y = 0;
		}
		public void mouseReleased (MouseEvent e){
			x = e.getX();
			y = e.getY();
			if (!check){
				if (y >= 196 && y < 614){
					if (x >= 0 && x < 430 && jpg == 1) check = true;
					else if (x >= 430 && x < 852 && jpg == 2) jpg = 0;
					else if (x >= 852 && x < 1290 && jpg == 3) check = true;
					else jpg = 0;
				}
				else jpg = 0;
			}
			else{
				if (y >= 16 && y <= 73 && x >= 18 && x <= 113) {
					check = false;
					jpg = 0;
				}
			}
			pressed = false;
			object.released(x,y,jpg, pressed, check);
			x = 0;
			y = 0;
		}	
	}
}

class GUIFrame extends JPanel {
	public static int x, y, jpg;
	public boolean check = false;
	public boolean pressed = false;
	
	public void framing(int xx, int yy, int jj, boolean pp){
		x = xx;
		y = yy;
		jpg = jj;
		pressed = pp;
		repaint();
	}
	
	public void released(int xx, int yy, int jj, boolean pp, boolean cc){
		x = xx;
		y = yy;
		jpg = jj;
		pressed = pp;
		check = cc;
		repaint();
	}
	
	public void paintComponent(Graphics g){
		super.paintComponent(g);
		ImageIcon i = new ImageIcon("GUI.jpg");
		if (!check){
			if (jpg == 0) i = new ImageIcon("GUI.jpg");
			else if (jpg == 1) i = new ImageIcon("GUI_DG_pressed.jpg");
			else if (jpg == 2) i = new ImageIcon("GUI_M_pressed.jpg");
			else if (jpg == 3) i = new ImageIcon("GUI_S_pressed.jpg");
		}
		else {
			if (jpg == 3){
				if (!pressed) i = new ImageIcon("GUI_Specs.jpg");
				else if (pressed) i = new ImageIcon("GUI_Specs_pressed.jpg");
			}
			if (jpg == 1){
				if (!pressed) i = new ImageIcon("GUI_DataNGraphs.jpg");
				else if (pressed) i = new ImageIcon("GUI_DataNGraphs_pressed.jpg");
			}
				
		}
        i.paintIcon(this, g, 0, 0);
		g.setColor(Color.red);
		g.drawString("x = " + x + ", y = " +y, 80, 80);
	}
}

		
		