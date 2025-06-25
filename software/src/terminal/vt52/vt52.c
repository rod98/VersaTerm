void INFLASHFUN terminal_receive_char_vt52(char c)
{
  static char start_char, row;

  switch( terminal_state )
    {
    case TS_NORMAL:
      {
        if( c==27 )
          terminal_state = TS_STARTCHAR;
        else
          terminal_process_text(c);
        
        break;
      }

    case TS_STARTCHAR:
      {
        terminal_state = TS_NORMAL;

        switch( c )
          {
          case 'A': 
            move_cursor_limited(cursor_row-1, cursor_col);
            break;

          case 'B': 
            move_cursor_limited(cursor_row+1, cursor_col);
            break;

          case 'C': 
            move_cursor_limited(cursor_row, cursor_col+1);
            break;

          case 'D': 
            move_cursor_limited(cursor_row, cursor_col-1);
            break;

          case 'E':
            framebuf_fill_screen(' ', color_fg, color_bg);
            // fall through

          case 'H': 
            move_cursor_limited(0, 0);
            break;

          case 'I': 
            move_cursor_wrap(cursor_row-1, cursor_col);
            break;

          case 'J':
            show_cursor(false);
            framebuf_fill_region(cursor_col, cursor_row, framebuf_get_ncols(cursor_row)-1, framebuf_get_nrows()-1, ' ', color_fg, color_bg);
            cur_attr = framebuf_get_attr(cursor_col, cursor_row);
            show_cursor(cursor_shown);
            break;

          case 'K':
            show_cursor(false);
            framebuf_fill_region(cursor_col, cursor_row, framebuf_get_ncols(cursor_row)-1, cursor_row, ' ', color_fg, color_bg);
            cur_attr = framebuf_get_attr(cursor_col, cursor_row);
            show_cursor(cursor_shown);
            break;

          case 'L':
          case 'M':
            show_cursor(false);
            framebuf_scroll_region(cursor_row, framebuf_get_nrows()-1, c=='M' ? 1 : -1, color_fg, color_bg);
            cur_attr = framebuf_get_attr(cursor_col, cursor_row);
            show_cursor(cursor_shown);
            break;

          case 'Y':
            start_char = c;
            row = 0;
            terminal_state = TS_READPARAM;
            break;
            
          case 'Z':
            send_string("\033/K");
            break;

          case 'b':
          case 'c':
            start_char = c;
            terminal_state = TS_READPARAM;
            break;

          case 'd':
            framebuf_fill_region(0, 0, cursor_col, cursor_row, ' ', color_fg, color_bg);
            init_cursor(cursor_col, cursor_row);
            break;
            
          case 'e':
            show_cursor(true);
            break;

          case 'f':
            show_cursor(false);
            break;

          case 'j':
            saved_col = cursor_col;
            saved_row = cursor_row;
            break;

          case 'k':
            move_cursor_limited(saved_row, saved_col);
            break;

          case 'l':
            framebuf_fill_region(0, cursor_row, framebuf_get_ncols(cursor_row)-1, cursor_row, ' ', color_fg, color_bg);
            init_cursor(0, cursor_row);
            break;

          case 'o':
            framebuf_fill_region(0, cursor_row, cursor_col, cursor_row, ' ', color_fg, color_bg);
            show_cursor(cursor_shown);
            break;

          case 'p':
            framebuf_set_screen_inverted(true);
            break;

          case 'q':
            framebuf_set_screen_inverted(false);
            break;

          case 'v':
            auto_wrap_mode = true;
            break;

          case 'w':
            auto_wrap_mode = false;
            break;

          case '<':
            terminal_reset();
            vt52_mode = false;
            break;
          }

        break;
      }

    case TS_READPARAM:
      {
        if( start_char=='Y' )
          {
            if( row==0 )
              row = c;
            else
              {
                if( row>=32 && c>=32 ) move_cursor_limited(row-32, c-32);
                terminal_state = TS_NORMAL;
              }
          }
        else if( start_char=='b' && c>=32 )
          {
            color_fg = (c-32) & 15;
            show_cursor(cursor_shown);
          }
        else if( start_char=='c' && c>=32 )
          {
            color_bg = (c-32) & 15;
            show_cursor(cursor_shown);
          }

        break;
      }
    }
}