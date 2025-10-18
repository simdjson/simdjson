/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
 */
function initMenu(relPath,searchEnabled,serverSide,searchPage,search) {
  function makeTree(data,relPath) {
    var result='';
    if ('children' in data) {
      result+='<ul>';
      for (var i in data.children) {
        var url;
        var link;
        link = data.children[i].url;
        if (link.substring(0,1)=='^') {
          url = link.substring(1);
        } else {
          url = relPath+link;
        }
        result+='<li><a href="'+url+'">'+
                                data.children[i].text+'</a>'+
                                makeTree(data.children[i],relPath)+'</li>';
      }
      result+='</ul>';
    }
    return result;
  }
  var searchBoxHtml;
  if (searchEnabled) {
    if (serverSide) {
      searchBoxHtml='<div id="MSearchBox" class="MSearchBoxInactive">'+
                 '<div class="left">'+
                  '<form id="FSearchBox" action="'+relPath+searchPage+
                    '" method="get"><span id="MSearchSelectExt">&#160;</span>'+
                  '<input type="text" id="MSearchField" name="query" value="" placeholder="'+search+
                    '" size="20" accesskey="S" onfocus="searchBox.OnSearchFieldFocus(true)"'+
                    ' onblur="searchBox.OnSearchFieldFocus(false)"/>'+
                  '</form>'+
                 '</div>'+
                 '<div class="right"></div>'+
                '</div>';
    } else {
      searchBoxHtml='<div id="MSearchBox" class="MSearchBoxInactive">'+
                 '<span class="left">'+
                  '<span id="MSearchSelect" onmouseover="return searchBox.OnSearchSelectShow()"'+
                     ' onmouseout="return searchBox.OnSearchSelectHide()">&#160;</span>'+
                  '<input type="text" id="MSearchField" value="" placeholder="'+search+
                    '" accesskey="S" onfocus="searchBox.OnSearchFieldFocus(true)" '+
                    'onblur="searchBox.OnSearchFieldFocus(false)" '+
                    'onkeyup="searchBox.OnSearchFieldChange(event)"/>'+
                 '</span>'+
                 '<span class="right"><a id="MSearchClose" '+
                  'href="javascript:searchBox.CloseResultsWindow()">'+
                  '<img id="MSearchCloseImg" border="0" src="'+relPath+
                  'search/close.svg" alt=""/></a>'+
                 '</span>'+
                '</div>';
    }
  }

  $('#main-nav').before('<div class="sm sm-dox"><input id="main-menu-state" type="checkbox"/>'+
                        '<label class="main-menu-btn" for="main-menu-state">'+
                        '<span class="main-menu-btn-icon"></span> '+
                        'Toggle main menu visibility</label>'+
                        '<span id="searchBoxPos1" style="position:absolute;right:8px;top:8px;height:36px;"></span>'+
                        '</div>');
  $('#main-nav').append(makeTree(menudata,relPath));
  $('#main-nav').children(':first').addClass('sm sm-dox').attr('id','main-menu');
  if (searchBoxHtml) {
    $('#main-menu').append('<li id="searchBoxPos2" style="float:right"></li>');
  }
  var $mainMenuState = $('#main-menu-state');
  var prevWidth = 0;
  if ($mainMenuState.length) {
    function initResizableIfExists() {
      if (typeof initResizable==='function') initResizable();
    }
    // animate mobile menu
    $mainMenuState.change(function(e) {
      var $menu = $('#main-menu');
      var options = { duration: 250, step: initResizableIfExists };
      if (this.checked) {
        options['complete'] = function() { $menu.css('display', 'block') };
        $menu.hide().slideDown(options);
      } else {
        options['complete'] = function() { $menu.css('display', 'none') };
        $menu.show().slideUp(options);
      }
    });
    // set default menu visibility
    function resetState() {
      var $menu = $('#main-menu');
      var $mainMenuState = $('#main-menu-state');
      var newWidth = $(window).outerWidth();
      if (newWidth!=prevWidth) {
        if ($(window).outerWidth()<768) {
          $mainMenuState.prop('checked',false); $menu.hide();
          $('#searchBoxPos1').html(searchBoxHtml);
          $('#searchBoxPos2').hide();
        } else {
          $menu.show();
          $('#searchBoxPos1').empty();
          $('#searchBoxPos2').html(searchBoxHtml);
          $('#searchBoxPos2').show();
        }
        if (typeof searchBox!=='undefined') {
          searchBox.CloseResultsWindow();
        }
        prevWidth = newWidth;
      }
    }
    $(window).ready(function() { resetState(); initResizableIfExists(); });
    $(window).resize(resetState);
  }
  $('#main-menu').smartmenus();
}
/* @license-end */
