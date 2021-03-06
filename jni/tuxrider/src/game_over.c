/* 
 * Tux Racer 
 * Copyright (C) 1999-2001 Jasmin F. Patry
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "tuxracer.h"
#include "audio.h"
#include "game_config.h"
#include "multiplayer.h"
#include "gl_util.h"
#include "fps.h"
#include "render_util.h"
#include "phys_sim.h"
#include "view.h"
#include "course_render.h"
#include "tux.h"
#include "tux_shadow.h"
#include "keyboard.h"
#include "loop.h"
#include "fog.h"
#include "viewfrustum.h"
#include "hud.h"
#include "game_logic_util.h"
#include "fonts.h"
#include "ui_mgr.h"
#include "joystick.h"
#include "part_sys.h"
#import "sharedGeneralFunctions.h"
#include "game_over.h"

#define NEXT_MODE RACE_SELECT

static bool_t aborted = False;
static bool_t race_won = False;
static char* friendsRanking;
static char* countryRanking;
static char* worldRanking;

static void mouse_cb( int button, int state, int x, int y )
{
#ifdef __APPLE__
    if (g_game.practicing && !g_game.race_aborted && g_game.race.name!=NULL && did_player_beat_best_results()  && g_game.rankings_displayed==false) {
        //Notify that a new best result is for the moment unsaved
        dirtyScores();
    }
    
    if (!g_game.race_aborted && g_game.practicing && plyrWantsToSaveOrDisplayRankingsAfterRace() && g_game.rankings_displayed==false) {
        saveAndDisplayRankings();
    }
    else 
    {
        set_game_mode( NEXT_MODE );
        winsys_post_redisplay();
    }
#else
    set_game_mode( NEXT_MODE );
    winsys_post_redisplay();
#endif
}


/*---------------------------------------------------------------------------*/
/*! 
  Draws the text for the game over screen
  \author  jfpatry
  \date    Created:  2000-09-24
  \date    Modified: 2000-09-24
*/
void draw_game_over_text( void )
{
    int w = getparam_x_resolution();
    int h = getparam_y_resolution();
    int x_org, y_org;
    int box_width, box_height;
    char *string;
    int string_w, asc, desc;
    char buff[BUFF_LEN];
    font_t *font;
    font_t *stat_label_font;
    player_data_t *plyr = get_player_data( local_player() );

    box_width = 200;
    box_height = 250;

    x_org = w/2.0 - box_width/2.0;
    y_org = h/2.0 - box_height/2.0;

    if ( !get_font_binding( "race_over", &font ) ) {
	print_warning( IMPORTANT_WARNING,
		       "Couldn't get font for binding race_over" );
    } else {
#ifdef __APPLE__
        if ( !g_game.race_aborted && g_game.practicing && g_game.needs_save_or_display_rankings) {
            string = "World rankings";
        } else string = "Race Over";
#else
        string = "Race Over";
#endif
	get_font_metrics( font, string, &string_w, &asc, &desc );
	
	glPushMatrix();
	{
	    glTranslatef( x_org + box_width/2.0 - string_w/2.0,
			  y_org + box_height - asc, 
			  0 );
	    bind_font_texture( font );
	    draw_string( font, string );
	}
	glPopMatrix();
    }

    /* If race was aborted, don't print stats, and if doesnt need to print rankings, dont print them */
#ifdef __APPLE__
    if ( !g_game.race_aborted && !g_game.needs_save_or_display_rankings) 
#else
    if ( !g_game.race_aborted) 
#endif
    {
        if ( !get_font_binding( "race_stats_label", &stat_label_font ) ||
            !get_font_binding( "race_stats", &font ) )
        {
            print_warning( IMPORTANT_WARNING,
                          "Couldn't get fonts for race stats" );
        } else {
            int asc2;
            int desc2;
            get_font_metrics( font, "", &string_w, &asc, &desc );
            get_font_metrics( stat_label_font, "", &string_w, &asc2, &desc2 );
            
            if ( asc < asc2 ) {
                asc = asc2;
            }
            if ( desc < desc2 ) {
                desc = desc2;
            }
            
            glPushMatrix();
            {
                int minutes;
                int seconds;
                int hundredths;
                
                glTranslatef( x_org,
                             y_org + 150,
                             0 );
                
                bind_font_texture( stat_label_font );
                draw_string( stat_label_font, "Time: " );
                
                get_time_components( g_game.time, &minutes, &seconds, &hundredths );
                
                sprintf( buff, "%02d:%02d:%02d", minutes, seconds, hundredths );
                
                bind_font_texture( font );
                draw_string( font, buff );
            }
            glPopMatrix();
            
            glPushMatrix();
            {
                glTranslatef( x_org,
                             y_org + 150 - (asc + desc),
                             0 );
                
                bind_font_texture( stat_label_font );
                draw_string( stat_label_font, "Fish: " );
                
                sprintf( buff, "%3d", plyr->herring );
                
                bind_font_texture( font );
                draw_string( font, buff );
            }
            glPopMatrix();
            
            glPushMatrix();
            {
                glTranslatef( x_org,
                             y_org + 150 - 2*(asc + desc),
                             0 );
                
                bind_font_texture( stat_label_font );
                draw_string( stat_label_font, "Score: " );
                
                sprintf( buff, "%6d", plyr->score );
                
                bind_font_texture( font );
                draw_string( font, buff );
            }
            glPopMatrix();
        }
    } 
#ifdef __APPLE__
    //display rankings if needed
    else if ( !g_game.race_aborted && g_game.practicing && g_game.needs_save_or_display_rankings) 
    {
        if ( !get_font_binding( "race_stats_label", &stat_label_font ) ||
            !get_font_binding( "race_stats", &font ) )
        {
            print_warning( IMPORTANT_WARNING,
                          "Couldn't get fonts for race stats" );
        } else {
            int asc2;
            int desc2;
            get_font_metrics( font, "", &string_w, &asc, &desc );
            get_font_metrics( stat_label_font, "", &string_w, &asc2, &desc2 );
            
            if ( asc < asc2 ) {
                asc = asc2;
            }
            if ( desc < desc2 ) {
                desc = desc2;
            }
            
            glPushMatrix();
            {
                glTranslatef( x_org,
                             y_org + 150,
                             0 );
                
                bind_font_texture( stat_label_font );
                draw_string( stat_label_font, "Friends : " );
                if (strcmp(friendsRanking,"Empty friends list.")==0) {
                    free(friendsRanking);
                    friendsRanking="No friends";
                }
                
                sprintf( buff, "%s", friendsRanking );
                
                bind_font_texture( font );
                draw_string( font, buff );
            }
            glPopMatrix();
            
            glPushMatrix();
            {
                glTranslatef( x_org,
                             y_org + 150 - (asc + desc),
                             0 );
                
                bind_font_texture( stat_label_font );
                draw_string( stat_label_font, "Country : " );
                
                sprintf( buff, "%s", countryRanking );
                
                bind_font_texture( font );
                draw_string( font, buff );
            }
            glPopMatrix();
            
            glPushMatrix();
            {
                glTranslatef( x_org,
                             y_org + 150 - 2*(asc + desc),
                             0 );
                
                bind_font_texture( stat_label_font );
                draw_string( stat_label_font, "World: " );
                
                sprintf( buff, "%s", worldRanking );
                
                bind_font_texture( font );
                draw_string( font, buff );
            }
            glPopMatrix();
        }
    }
#endif

    if ( g_game.race_aborted && !g_game.race_time_over) {
	string = "Race aborted.";
    } else if ( g_game.race_aborted && g_game.race_time_over) {
        string = "Time is up.";
    } else if ( ( g_game.practicing || is_current_cup_complete() ) &&
		did_player_beat_best_results() ) 
    {
#ifdef __APPLE__
        if ( !g_game.race_aborted && g_game.practicing && g_game.needs_save_or_display_rankings) {
            string = "";
        } else string = "You beat your best score!";
#else
        string = "You beat your best score!";
#endif
    } else if ( g_game.practicing || is_current_cup_complete() ) {
	string = "";
    } else if ( race_won && is_current_race_last_race_in_cup() ) {
	string = "Congratulations! You won the cup!";
    } else if ( race_won ) {
	string = "You advanced to the next race!";
    } else {
	string = "You didn't advance.";
    }

    if ( !get_font_binding( "race_result_msg", &font ) ) {
	print_warning( IMPORTANT_WARNING, 
		       "Couldn't get font for binding race_result_msg" );
    } else {
	get_font_metrics( font, string, &string_w, &asc, &desc );
	glPushMatrix();
	{
	    glTranslatef( x_org + box_width/2. - string_w/2.,
			  y_org + desc,
			  0 );
	    bind_font_texture( font );
	    draw_string( font, string );
	}
	glPopMatrix();
    }
}

#ifdef __APPLE__  
//this function is called from scoreController.m in the function treatError
void displaySavedAndRankings(const char* msg, const char* friends, const char* country, const char* world) {
    friendsRanking = strdup(friends);
    countryRanking = strdup(country);
    worldRanking = strdup(world);
}

void saveAndDisplayRankings() { 
    //save score online if a best resul was established
    if (g_game.practicing && !g_game.race_aborted && g_game.race.name!=NULL && did_player_beat_best_results()) {
        int minutes;
        int seconds;
        int hundredths;
        player_data_t *plyr = get_player_data( local_player() );
        get_time_components( g_game.time, &minutes, &seconds, &hundredths);
        //if the player choosed in his prefs not to save score online after ending a race but just to display rankings, the function below
        //will detect this case and redirect to the function displayRankingsAfterRace
        saveScoreOnlineAfterRace(g_game.race.name,plyr->score,plyr->herring,minutes,seconds,hundredths);
    } 
    //else display world rankings for this score
    else if (g_game.practicing && !g_game.race_aborted && g_game.race.name!=NULL && !did_player_beat_best_results()) {
        int minutes;
        int seconds;
        int hundredths;
        player_data_t *plyr = get_player_data( local_player() );
        get_time_components( g_game.time, &minutes, &seconds, &hundredths);
        displayRankingsAfterRace(g_game.race.name,plyr->score,plyr->herring,minutes,seconds,hundredths);
    }
}
#endif

void game_over_init(void) 
{
    winsys_set_display_func( main_loop );
    winsys_set_idle_func( main_loop );
    winsys_set_reshape_func( reshape );
    winsys_set_mouse_func( mouse_cb );
    winsys_set_motion_func( ui_event_motion_func );
    winsys_set_passive_motion_func( ui_event_motion_func );

    halt_sound( "flying_sound" );
    halt_sound( "rock_sound" );
    halt_sound( "ice_sound" );
    halt_sound( "snow_sound" );

    play_music( "game_over" );

    aborted = g_game.race_aborted;

    if ( !aborted ) {
        update_player_score( get_player_data( local_player() ) );
    }

    if ( !g_game.practicing ) {
        race_won = was_current_race_won();
    }
    
    g_game.needs_save_or_display_rankings=false;
    g_game.rankings_displayed=false;
}

void game_over_loop( scalar_t time_step )
{
    player_data_t *plyr = get_player_data( local_player() );
    int width, height;
    width = getparam_x_resolution();
    height = getparam_y_resolution();

    check_gl_error();

    /* Check joystick */
    if ( is_joystick_active() ) {
	update_joystick();

	if ( is_joystick_continue_button_down() )
	{
	    set_game_mode( NEXT_MODE );
	    winsys_post_redisplay();
	    return;
	}
    }

    new_frame_for_fps_calc();

    update_audio();

    clear_rendering_context();

    setup_fog();

    update_player_pos( plyr, 0 );
    update_view( plyr, 0 );

    setup_view_frustum( plyr, NEAR_CLIP_DIST, 
			getparam_forward_clip_distance() );

    draw_sky(plyr->view.pos);

    draw_fog_plane();

    set_course_clipping( True );
    set_course_eye_point( plyr->view.pos );
    setup_course_lighting();
    render_course();
    draw_trees();

    if ( getparam_draw_particles() ) {
	draw_particles( plyr );
    }

    draw_tux();
    draw_tux_shadow();

    set_gl_options( GUI );

    ui_setup_display();

    draw_game_over_text();

#ifndef __APPLE__
    draw_hud( plyr );
#endif
    reshape( width, height );

    winsys_swap_buffers();
} 

START_KEYBOARD_CB( game_over_cb )
{
    if ( release ) return;
#ifdef __APPLE__

    if (g_game.practicing && !g_game.race_aborted && g_game.race.name!=NULL && did_player_beat_best_results()  && g_game.rankings_displayed==false) {
        //Notify that a new best result is for the moment unsaved
        dirtyScores();
    }
    
    if (!g_game.race_aborted && g_game.practicing && plyrWantsToSaveOrDisplayRankingsAfterRace() && g_game.rankings_displayed==false) {
        saveAndDisplayRankings();
    }
    else 
    {
        set_game_mode( NEXT_MODE );
        winsys_post_redisplay();
    }
#else
    set_game_mode( NEXT_MODE );
    winsys_post_redisplay();
#endif

}
END_KEYBOARD_CB

void game_over_register()
{
    int status = 0;

    status |= add_keymap_entry( GAME_OVER, 
				DEFAULT_CALLBACK, NULL, NULL, game_over_cb );

    check_assertion( status == 0, "out of keymap entries" );

    register_loop_funcs( GAME_OVER, game_over_init, game_over_loop, NULL );
}


