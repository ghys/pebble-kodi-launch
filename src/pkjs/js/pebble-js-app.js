var options = JSON.parse(localStorage.getItem('options'));
var refresh_timeout;

Pebble.addEventListener('showConfiguration', function() {
  var url = 'https://rawgit.com/ghys/pebble-kodi-launch/master/config/config.html';
  console.log('Showing configuration page: ' + url);

  Pebble.openURL(url);
});

function sendRequest(method, params, callback) {
    var req = new XMLHttpRequest();
    req.onload = function (e) {
   
      if (req.readyState == 4 && req.status == 200) {
        callback(null, JSON.parse(req.responseText));
        //console.log("responseText: " + req.responseText.substring(1, 4)); 
      } else {
        callback(req.status, null);
      }
        
    };
    
    var payload = {
        jsonrpc: "2.0",
        id: 1,
        method: method,
        params: params
    };
    
    req.open('POST', "http://" + options.kodi_ip + "/jsonrpc");
    if (options.kodi_auth) req.setRequestHeader("Authorization", "Basic " + options.kodi_auth); 
    req.setRequestHeader("Content-type", "application/json"); 
    req.send(JSON.stringify(payload));
}

var send = function (dict) {
  // Send to watchapp
  //console.log(JSON.stringify(dict));
  Pebble.sendAppMessage(dict, function() {
    //console.log('Send successful: ' + JSON.stringify(dict));
  }, function(e) {
    console.log('Send failed! ' + JSON.stringify(e));
  });
}; 

function pad(num, size) {
    var s = num+"";
    while (s.length < size) s = "0" + s;
    return s;
}

var sendCurrentStatus = function(data, data2, data3, data4) {
    var dict = {};
    dict.volume= data.result.volume.toString();
    dict.mute = (data.result.muted) ? "Mute" : "";
    dict.playback_main = (data3.result.item.type === "episode") ? data3.result.item.showtitle
    : data3.result.item.label;
    dict.playback_sub = (data3.result.item.type === "episode") ? data3.result.item.season.toString() +
        'x' + data3.result.item.episode.toString() + ' ' + data3.result.item.title
    : (data3.result.item.type === "song") ? data3.result.item.artist[0]
    : (data3.result.item.type === "channel") ? data3.result.item.title : "";
    if (data4.result) {
        dict.playback_status = "Playing";
        dict.playback_elapsed = ((data4.result.time.hours) ? data4.result.time.hours + ":" : "") +
            pad(data4.result.time.minutes, 2) + ":" + pad(data4.result.time.seconds, 2);
        if (data4.result.totaltime.minutes || data4.result.totaltime.seconds)
            dict.playback_elapsed += " / " + ((data4.result.totaltime.hours) ? data4.result.totaltime.hours + ":" : "") +
                pad(data4.result.totaltime.minutes, 2) + ":" + pad(data4.result.totaltime.seconds, 2);

            
        dict.input_title = (data3.result.item.type === "episode") ? "TV Show" :
                           (data3.result.item.type === "channel") ? "Live TV" :
                           (data3.result.item.type === "song") ? "Music" :
                           (data3.result.item.type === "movie") ? "Movie" :
                            data3.result.item.type;
    } else {
        dict.playback_status = "Stopped";
        dict.playback_elapsed = "";
        dict.input_title = "";
    }

    send(dict);    
};

var getBasicInfo = function() {
    sendRequest("Application.GetProperties", {
        "properties": ["volume", "muted", "name", "version"]
    }, function (err, data) {
        if (data) {
            sendRequest("Player.GetActivePlayers", {}, function (err2, data2) {
                if (data2 && data2.result.length > 0) {
                    sendRequest("Player.GetItem", {
                        "playerid": data2.result[0].playerid,
                        "properties": ["title", "album", "artist", "track", "season", "episode", "showtitle"]
                    }, function (err3, data3) {
                        if (data3) {
                            sendRequest("Player.GetProperties", {
                                "playerid": data2.result[0].playerid,
                                "properties": ["live", "time", "totaltime"]
                            }, function (err4, data4) {
                                if (data4) {
                                    sendCurrentStatus(data, data2, data3, data4);
                                } else {                                    
                                    send({ERROR: "Player.GetProperties Error " + err4});
                                }
                            });
                        } else {
                            send({ERROR: "Player.GetItem Error " + err3});
                        }
                    });
                } else {
                    sendCurrentStatus(data, data2, {result: { item: {label: "Nothing playing", type: "N/A"}}}, {});
                }
            });
        }
    });
};

var getRecentEpisodes = function() {
    sendRequest("VideoLibrary.GetRecentlyAddedEpisodes", {
        "properties": ["showtitle", "runtime", "tvshowid"]
    }, function (err, data) {
        if (data) {
            var dict = { nb_items: data.result.episodes.length };
            
            for (var i = 0; i < dict.nb_items; i++) {
                dict[(10000 + (i * 10) + 0).toString()] = data.result.episodes[i].episodeid;
                dict[(10000 + (i * 10) + 1).toString()] = data.result.episodes[i].showtitle;
                dict[(10000 + (i * 10) + 2).toString()] = data.result.episodes[i].label;
                //dict[(10000 + (i * 10) + 3).toString()] = data.result.episodes[i].runtime;
                //dict[(10000 + (i * 10) + 4).toString()] = data.result.episodes[i].tvshowid;
            }
            
            console.log(JSON.stringify(dict));
            
            send(dict);
        } else {
            var errdict = { ERROR: "Error " + err };
            send(errdict);
        }
    });
};

var getTVShows = function(sort, unwatchedonly) {
    sendRequest("VideoLibrary.GetTVShows", {
        "properties": ["season", "episode", "lastplayed", "watchedepisodes"],
        "sort": sort,
        "limits": { "start": 0, "end": 25 }
    }, function (err, data) {
        if (data) {
            var nb_items = 0;
            console.log(JSON.stringify(data));
            var dict = {};
            
            for (var i = 0; i < data.result.tvshows.length; i++) {
                var unwatched = data.result.tvshows[i].episode - data.result.tvshows[i].watchedepisodes;
                if (unwatchedonly && unwatched === 0) continue;
                
                dict[(10000 + (nb_items * 10) + 0).toString()] = data.result.tvshows[i].tvshowid;
                dict[(10000 + (nb_items * 10) + 1).toString()] = data.result.tvshows[i].label;
                
                if (unwatchedonly)
                    dict[(10000 + (nb_items * 10) + 2).toString()] = unwatched + " episode" + (unwatched === 1 ? "" : "s") + " to watch";
                else
                    dict[(10000 + (nb_items * 10) + 2).toString()] = data.result.tvshows[i].season.toString() +
                                                                    (data.result.tvshows[i].season === 1 ? ' season, ' : ' seasons, ') +
                                                                     data.result.tvshows[i].episode.toString() +
                                                                    (data.result.tvshows[i].episode === 1 ? ' episode' : ' episodes');
                nb_items++;
                }
            
            dict.nb_items = nb_items;
            console.log(JSON.stringify(dict));
            send(dict);
        } else {
            var errdict = { ERROR: "Error " + err };
            send(errdict);
        }
    });
};

var getSeasons = function(tvshowid, sort) {
    sendRequest("VideoLibrary.GetSeasons", {
        "tvshowid": tvshowid,
        "properties": ["season", "episode", "watchedepisodes"],
        "sort": sort,
        "limits": { "start": 0, "end": 25 }
    }, function (err, data) {
        if (data) {
            console.log(JSON.stringify(data));
            var dict = { nb_items: data.result.seasons.length };
            
            for (var i = 0; i < dict.nb_items; i++) {
                dict[(10000 + (i * 10) + 0).toString()] = data.result.seasons[i].season;
                dict[(10000 + (i * 10) + 1).toString()] = "Season " + data.result.seasons[i].season.toString();
                var unwatched = data.result.seasons[i].episode - data.result.seasons[i].watchedepisodes;
                if (unwatched === 0)
                    dict[(10000 + (i * 10) + 2).toString()] = data.result.seasons[i].episode.toString() + " episode" +
                                                          (data.result.seasons[i].episode === 1 ? "" : "s") +
                                                          " watched";
                else
                    dict[(10000 + (i * 10) + 2).toString()] = unwatched + " episode" + (unwatched === 1 ? "" : "s") + " to watch";
                        
            }
            
            console.log(JSON.stringify(dict));
            
            send(dict);
        } else {
            var errdict = { ERROR: "Error " + err };
            send(errdict);
        }
    });
};

var getEpisodes = function(tvshowid, season, sort) {
    sendRequest("VideoLibrary.GetEpisodes", {
        "tvshowid": tvshowid,
        "season": season,
        "properties": ["season", "episode", "firstaired", "playcount"],
        sort: sort,
        "limits": { "start": 0, "end": 30 }
    }, function (err, data) {
        if (data) {
            console.log(JSON.stringify(data));
            var dict = { nb_items: data.result.episodes.length };
            
            for (var i = 0; i < dict.nb_items; i++) {
                dict[(10000 + (i * 10) + 0).toString()] = data.result.episodes[i].episodeid;
                dict[(10000 + (i * 10) + 1).toString()] = data.result.episodes[i].label.substring(0, 20);
                dict[(10000 + (i * 10) + 2).toString()] = data.result.episodes[i].firstaired.toString() +
                    (data.result.episodes[i].playcount > 0 ? " \uD83D\uDC4D" : "");
            }
            
            console.log(JSON.stringify(dict));
            
            send(dict);
        } else {
            var errdict = { ERROR: "Error " + err };
            send(errdict);
        }
    });
};

var playEpisode = function(episodeid) {
    sendRequest("VideoLibrary.GetEpisodeDetails", {
       "episodeid": episodeid,
       "properties": ["file"],
    }, function (err, data) {
        if (data) {
            console.log("Playing " + data.result.episodedetails.label);
            sendRequest("Player.Open", {
                "item": { "file": data.result.episodedetails.file }
            }, function (data2, err2) {
            });
        } else {
            var errdict = { ERROR: "Error " + err };
            send(errdict);
        }
    });
};


var getMovies = function(sort, unwatchedonly) {
    sendRequest("VideoLibrary.GetMovies", {
        "properties": ["year", "country", "rating"],
        "sort": sort,
        filter: (unwatchedonly ? {"field": "playcount", "operator": "is", "value": "0"}
                 : {"field": "title", "operator": "isnot", "value": ""}),
        "limits": { "start": 0, "end": 25 }
    }, function (err, data) {
        if (data) {
            console.log(JSON.stringify(data));
            var dict = { nb_items: data.result.movies.length };
            
            for (var i = 0; i < dict.nb_items; i++) {
                dict[(10000 + (i * 10) + 0).toString()] = data.result.movies[i].movieid;
                dict[(10000 + (i * 10) + 1).toString()] = data.result.movies[i].label;
                dict[(10000 + (i * 10) + 2).toString()] = data.result.movies[i].year.toString() + 
                    ((data.result.movies[i].rating) ? ", " + (Math.round(data.result.movies[i].rating*10)/10).toString() + "/10" : "");
            }
            
            console.log(JSON.stringify(dict));
            
            send(dict);
        } else {
            var errdict = { ERROR: "Error " + err };
            send(errdict);
        }
    });
};

var getChannelGroups = function() {
    sendRequest("PVR.GetChannelGroups", {
        "channeltype": "tv",
        "limits": { "start": 0, "end": 25 }
    }, function (err, data) {
        if (data) {
            console.log(JSON.stringify(data));
            var dict = { nb_items: data.result.channelgroups.length };
            
            for (var i = 0; i < dict.nb_items; i++) {
                dict[(10000 + (i * 10) + 0).toString()] = data.result.channelgroups[i].channelgroupid;
                dict[(10000 + (i * 10) + 1).toString()] = data.result.channelgroups[i].label;
            }
            
            console.log(JSON.stringify(dict));
            
            send(dict);
        } else {
            var errdict = { ERROR: "Error " + err };
            send(errdict);
        }
    });
};

var getChannels = function(channelgroupid) {
    sendRequest("PVR.GetChannels", {
        "channelgroupid": channelgroupid,
        //"properties": ["lastplayed"],
        "limits": { "start": 0, "end": 50 }
    }, function (err, data) {
        if (data) {
            console.log(JSON.stringify(data));
            var dict = { nb_items: data.result.channels.length };
            
            for (var i = 0; i < dict.nb_items; i++) {
                dict[(10000 + (i * 10) + 0).toString()] = data.result.channels[i].channelid;
                dict[(10000 + (i * 10) + 1).toString()] = data.result.channels[i].label;
            }
            
            console.log(JSON.stringify(dict));
            
            send(dict);
        } else {
            var errdict = { ERROR: "Error " + err };
            send(errdict);
        }
    });
};


var playMovie = function(movieid) {
    sendRequest("VideoLibrary.GetMovieDetails", {
       "movieid": movieid,
       "properties": ["file"],
    }, function (err, data) {
        if (data) {
            console.log("Playing " + data.result.moviedetails.label);
            sendRequest("Player.Open", {
                "item": { "file": data.result.moviedetails.file }
            }, function (data2, err2) {
            });
        } else {
            var errdict = { ERROR: "Error " + err };
            send(errdict);
        }
    });
};

var playChannel = function(channelid) {
    console.log("Playing channelid: " + channelid);
    sendRequest("Player.Open", {
        "item": { "channelid": channelid }
    }, function (data, err) {
    });
};


var sendKeypress = function(key) {
    sendRequest("Input." + key, {}, function (err, data) {
        if (data) console.log("Keypress sent: " + key);
        if (err) console.log("Keypress not sent: " + key);
    });
};

var sendToCurrentPlayer = function(method, params) {
    sendRequest("Player.GetActivePlayers", {}, function (err, data) {
        if (data && data.result.length > 0) {
            params.playerid = data.result[0].playerid;
            sendRequest(method, params, function(err2, data2) {
                if (data2) {
                    // OK
                } else {
                    //send({ERROR: method + " Error " + err2});
                }
            });
        } else {
            //send({ERROR: "Player.GetActivePlayers Error " + err});
        }
    });
};

var setVolume = function(incdec) {
    sendRequest("Application.SetVolume", {
        "volume": incdec
    }, function (err, data) {
    });
};

var toggleMute = function() {
    sendRequest("Application.SetMute", {
        "mute": "toggle"
    }, function (err, data) {
    });
};


Pebble.addEventListener("appmessage", function(e) {
    //console.log("message received: " + JSON.stringify(e.payload));
    if (e.payload.data_request) {
        if (e.payload.data_request === "getbasicinfo") {
            refresh_timeout = setInterval(getBasicInfo, 3000);
            getBasicInfo();
        } else {
            clearTimeout(refresh_timeout);            
        }
        
        switch (e.payload.data_request) {
            case "recentepisodes":
                getRecentEpisodes();
                break;
            case "showsbytitle":
                getTVShows({"method": "title"});
                break;
            case "lastplayedshows":
                getTVShows({"method": "lastplayed", "order": "descending"}, false);
                break;
            case "unwatchedepisodes":
                getTVShows({"method": "dateadded", "order": "descending"}, true);
                break;
            case "recentmovies":
                getMovies({"method": "dateadded", "order": "descending"}, false);
                break;
            case "lastplayedmovies":
                getMovies({"method": "lastplayed", "order": "descending"}, false);
                break;
            case "unwatchedmovies":
                getMovies({"method": "dateadded", "order": "descending"}, true);
                break;
            case "seasons":
                if (e.payload.data_request_id)
                    getSeasons(e.payload.data_request_id, {});
                break;
            case "episodes":
                if (e.payload.data_request_id && e.payload.data_request_id2)
                    getEpisodes(e.payload.data_request_id, e.payload.data_request_id2, {});
                break;
            case "channelgroups":
                getChannelGroups();
                break;
            case "channels":
                if (e.payload.data_request_id)
                    getChannels(e.payload.data_request_id, {});
                break;
            default:
                console.log("Invalid data request");
        }
    } else if (e.payload.nav_keypress) {
        switch (e.payload.nav_keypress.toString()) {
            case "1":
                sendKeypress("Left");
                break;
            case "2":
                sendKeypress("Right");
                break;
            case "3":
                sendKeypress("Up");
                break;
            case "4":
                sendKeypress("Down");
                break;
            case "5":
                sendKeypress("Select");
                break;
            case "6":
                sendKeypress("Back");
                break;
            case "7":
                sendKeypress("ContextMenu");
                break;
            case "8":
                sendKeypress("ShowOSD");
                break;
            case "9":
                sendKeypress("ShowCodec");
                break;
            case "10":
                sendKeypress("Info");
                break;
        }
    } else if (e.payload.play_episode) {
        playEpisode(e.payload.play_episode);
    } else if (e.payload.play_movie) {
        playMovie(e.payload.play_movie);
    } else if (e.payload.play_channel) {
        playChannel(e.payload.play_channel);
    } else if (e.payload.play_pause) {
        sendToCurrentPlayer("Player.PlayPause", {play: "toggle"});
    } else if (e.payload.stop) {
        sendToCurrentPlayer("Player.Stop", {});
    } else if (e.payload.skip_rev) {
        sendToCurrentPlayer("Player.Seek", {value: "smallbackward"});
    } else if (e.payload.skip_fwd) {
        sendToCurrentPlayer("Player.Seek", {value: "smallforward"});
    } else if (e.payload.goto_previous) {
        sendToCurrentPlayer("Player.GoTo", {to: "previous"});
    } else if (e.payload.goto_next) {
        sendToCurrentPlayer("Player.GoTo", {to: "next"});
    } else if (e.payload.volume_down) {
        setVolume("decrement");
    } else if (e.payload.volume_up) {
        setVolume("increment");
    } else if (e.payload.mute_toggle) {
        toggleMute();
    } else if (e.payload.system_action) {
        switch (e.payload.system_action) {
            case "togglefullscreen":
                sendRequest("GUI.SetFullscreen", { "fullscreen": "toggle" });
                break;
            case "togglepartymode":
                sendToCurrentPlayer("Player.SetPartymode", { "partymode": "toggle" });
                break;
            case "libraryscan":
                sendRequest("VideoLibrary.Scan");
                break;
            case "toggleinfo":
                sendKeypress("Info");
                break;
            case "togglecodec":
                sendKeypress("ShowCodec");
                break;
            case "suspend":
                sendRequest("System.Suspend");
                break;
            case "reboot":
                sendRequest("System.Reboot");
                break;
            case "shutdown":
                sendRequest("System.Shutdown");
                break;
        }
    }
});


Pebble.addEventListener('webviewclosed', function(e) {
  if (e.response) { 
    options = JSON.parse(decodeURIComponent(e.response)); 
    localStorage.setItem('options', JSON.stringify(options)); 

    getBasicInfo();
  }
});


Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
  console.log('options:' + JSON.stringify(options));
  
  if (!options || !options.kodi_ip) {
    send({'ERROR': 'Please set up\nIP address\nin app settings'});
  }
  
  refresh_timeout = setInterval(getBasicInfo, 3000);
  getBasicInfo();
  
});
