
static void INFLASHFUN terminal_receive_char_petscii(uint8_t c)
{
  static uint8_t inserted = 0;
  static bool quoteMode = false;

  if( c>=192 )
    {
      if     ( c<=223 ) c -= 96;
      else if( c<=254 ) c -= 64;
      else if( c==255 ) c  = 126;
    }

  if( c==34 ) quoteMode=!quoteMode;

  if( (quoteMode || inserted>0) )
    {
      uint8_t cc = 0;
      if( c<32 && c!=13 && (!quoteMode || c!=20) )
        {
          switch( c )
            {
            case 27: cc = 0x5B; break;
            case 28: cc = 0x9A; break;
            case 29: cc = 0x5D; break;
            case 30: cc = 0x98; break;
            case 31: cc = 0x99; break;
            default: cc = petscii_lower_case_charset ? c+96 : c+64; break;
            }
        }
      else if( c>=128 && c<160 && (quoteMode || c!=148) )
        {
          switch( c )
            {
            case 155: cc = 0x9B; break;
            case 156: cc = 0x9C; break;
            case 157: cc = 0x9D; break;
            case 158: cc = petscii_lower_case_charset ? 0x9E : 0xCE; break;
            case 159: cc = petscii_lower_case_charset ? 0x9F : 0xDF; break;
            default:  cc = petscii_lower_case_charset ? c-64 : c+64; break;
            }
        }
          
      if( cc>0 )
        {
          uint8_t a = attr;
          attr |= ATTR_INVERSE;
          print_char_petscii(cc);
          if( inserted>0 ) inserted--;
          attr = a;
          return;
        }
    }
      
  switch( c )
    {
    case 5: // WHITE
      color_fg = 1;
      break;

    case 10:  // LF
    case 13:  // CR
    case 141: // shift+CR
      {
        switch( c==10 ? config_get_terminal_lf() : config_get_terminal_cr() )
          {
          case 1: move_cursor_wrap(cursor_row, 0); break;
          case 2: move_cursor_wrap(cursor_row+1, cursor_col); break;
          case 3: move_cursor_wrap(cursor_row+1, 0); break;
          }
        if( c!=10 ) { inserted = 0; quoteMode = false; attr &= ~ATTR_INVERSE; }
        break;
      }

    case 14: // Switch to lower case character set
      petscii_lower_case_charset = true;
      for(uint8_t row=0; row<framebuf_get_nrows(); row++)
        for(uint8_t col=0; col<framebuf_get_ncols(col); col++)
          {
            char c = framebuf_get_char(col, row);
            if( c>='A' && c<='Z' )
              c += 32;
            else if( c>='A'+128 && c<='Z'+128 ) 
              c -= 128;
            else if( c==0xDE || c==0xDF || c==0xE9 || c==0xFA )
              c -= 64;
            framebuf_set_char(col, row, c);
          }
      break;

    case 17: // cursor down
      move_cursor_wrap(cursor_row+1, cursor_col);
      break;

    case 18: // enable reverse character mode
      attr |= ATTR_INVERSE;
      break;

    case 19: // cursor home
      move_cursor_wrap(0, 0);
      break;

    case 20: // backspace/delete
      if( cursor_col>0 || cursor_row>0 )
        {
          move_cursor_wrap(cursor_row, cursor_col-1);
          framebuf_delete(cursor_col, cursor_row, 1, color_fg, color_bg);
          cur_attr = framebuf_get_attr(cursor_col, cursor_row);
          show_cursor(cursor_shown);
        }
      break;

    case 28: // red
      color_fg = 2;
      break;

    case 29: // cursor right
      move_cursor_wrap(cursor_row, cursor_col+1);
      break;
      
    case 30: // green
      color_fg = 5;
      break;

    case 31: // blue
      color_fg = 6;
      break;

    case 129: // orange
      color_fg = 8;
      break;

    case 142: // Switch to upper case character set
      petscii_lower_case_charset = false;
      for(uint8_t row=0; row<framebuf_get_nrows(); row++)
        for(uint8_t col=0; col<framebuf_get_ncols(col); col++)
          {
            char c = framebuf_get_char(col, row);
            if( c>='a' && c<='z' ) 
              c -= 32;
            else if( c>='A' && c<='Z' )
              c += 128;
            else if( c==0x9E || c==0x9F || c==0xA9 || c==0xBA )
              c += 64;
            framebuf_set_char(col, row, c);
          }
      break;

    case 144: // black
      color_fg = 0;
      break;

    case 145: // cursor up
      move_cursor_limited(cursor_row-1, cursor_col);
      break;

    case 146: // disable reverse character mode
      attr &= ~ATTR_INVERSE;
      break;

    case 147: // clear screen
      terminal_clear_screen();
      break;

    case 148: // insert
      show_cursor(false);
      framebuf_insert(cursor_col, cursor_row, 1, color_fg, color_bg);
      cur_attr = framebuf_get_attr(cursor_col, cursor_row);
      show_cursor(cursor_shown);
      inserted++;
      break;

    case 149: // brown
      color_fg = 9;
      break;

    case 150: // light red
      color_fg = 10;
      break;

    case 151: // dark grey
      color_fg = 11;
      break;

    case 152: // grey
      color_fg = 12;
      break;

    case 153: // light green
      color_fg = 13;
      break;

    case 154: // light blue
      color_fg = 14;
      break;

    case 155: // light gray
      color_fg = 15;
      break;

    case 156: // purple
      color_fg = 4;
      break;

    case 157: // cursor left
      if( cursor_row>0 )
        move_cursor_wrap(cursor_row, cursor_col-1);
      else
        move_cursor_limited(cursor_row, cursor_col-1);
      break;

    case 158: // yellow
      color_fg = 7;
      break;

    case 159: // cyan
      color_fg = 3;
      break;

    default:
      {
        if( c>=65 && c<=90 && petscii_lower_case_charset )
          c += 32;
        else if( c>=97 && c<=122 )
          c  = petscii_lower_case_charset ? c-32 : c+96;
        else if( c>=149 && c<=191 && c!=169 && c!=186 )
          c += 64; // more PETSCII graphics characters
        else if( c>=133 && c<=140 )
          c = 0; // function keys (no function in terminal and not printable)
        else
          {
            switch( c )
              {
              case  92: c = 0x9A; break; // pound symbol
              case  94: c = 0x98; break; // up arrow
              case  95: c = 0x99; break; // left arrow
              case  96: c = 0xC0; break; // middle horizontal line
              case 123: c = 0x9B; break; // cross
              case 124: c = 0x9C; break; // left checkerboard
              case 125: c = 0x9D; break; // middle vertical line
              case 126: c = petscii_lower_case_charset ? 0x9E : 0xDE ; break; // full checkerboard / pi
              case 127: c = petscii_lower_case_charset ? 0x9F : 0xDF ; break; // down diagonals / top right triangle
              case 169: c = petscii_lower_case_charset ? 0xA9 : 0xE9 ; break; // up diagonals / top left triangle
              case 186: c = petscii_lower_case_charset ? 0xBA : 0xFA ; break; // checkmark / bottom right corner
              }
          }

        if( c>0 ) print_char_petscii(c);
        if( inserted>0 ) inserted--;
        break;
      }
    }
}