/* XTools.pmod
 *
 * $Id: XTools.pmod,v 1.3 1998/04/06 15:48:04 nisse Exp $
 *
 * Various tools that are higher level than raw X, but are lower level
 * than widgets.
 */

/*
 *    px, a Pike interface to the X Window System
 *
 *    Copyright (C) 1998, Niels M�ller, Per Hedbor, Marcus Comstedt,
 *    Pontus Hagland, David Hedbor.
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA 
 */

/* Questions, bug fixes and bug reports can be sent to the pike
 * mailing list, pike@idonex.se, or to the athors (see AUTHORS for
 * email addresses. */

/* Steals and processes mousebutton events */
class Button
{
  object window;
  constant style = 1;
  int pressed; // button is pressed
  int inside;  // pointer is inside window
  int button;  // The number of the mouse button used
  
  function(object, int, mapping:void) redraw_callback;
  function(object:void) clicked_callback;

  void button_exposed(mapping event)
  {
    redraw_callback(this_object(), pressed && (!style || inside), event);
  }
  
  mapping button_pressed(mapping event)
  {
    werror(sprintf("Button %d pressed.\n", event->detail));
    if (event->detail == button)
      {
	pressed = 1;
	inside = 1;
	redraw_callback(this_object(), 1, 0);
	
	return 0;
      }
    else
      return event;
  }

  mapping button_released(mapping event)
  {
    if (event->detail == button)
      {
	pressed = 0;
	redraw_callback(this_object(), 0, 0);
	if (inside)
	  clicked_callback(this_object());
	return 0;
      }
    else 
      return event;
  }

  mapping window_entered(mapping event)
  {
    inside = 1;
    if (pressed && style)
      redraw_callback(this_object(), 1, 0);
    return 0;
  }

  mapping window_left(mapping event)
  {
    inside = 0;
    if (pressed && style)
      redraw_callback(this_object(), 0, 0);
    return 0;
  }
  
  void create(object w, int|void b)
  {
    window = w;
    button = b || 1;

    window->SelectInput("Exposure",
			"ButtonPress", "ButtonRelease",
			"EnterWindow", "LeaveWindow");
    // window->GrabButton(button, 0, "EnterWindow", "LeaveWindow");
    window->set_event_callback("Expose", button_exposed);
    window->set_event_callback("ButtonPress", button_pressed);
    window->set_event_callback("ButtonRelease", button_released);
    window->set_event_callback("EnterNotify", window_entered);
    window->set_event_callback("LeaveNotify", window_left);
  }
}
  
class Uglier_button
{
  inherit Button;
  constant style = 0;
}
