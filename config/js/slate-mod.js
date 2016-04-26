
itemIcon: function(){

  
var layout  = [
["icon-000.pgn","icon-001.pgn","icon-002.pgn","icon-003.pgn","icon-004.pgn","icon-005.pgn","icon-006.pgn","icon-007.pgn",],
["icon-008.pgn","icon-009.pgn","icon-010.pgn","icon-011.pgn","icon-012.pgn","icon-013.pgn","icon-014.pgn","icon-015.pgn",],
["icon-016.pgn","icon-017.pgn","icon-018.pgn","icon-019.pgn","icon-020.pgn","icon-021.pgn","icon-022.pgn","icon-023.pgn",],
["icon-024.pgn","icon-025.pgn","icon-026.pgn","icon-027.pgn","icon-028.pgn","icon-029.pgn","icon-030.pgn","icon-031.pgn",],
["icon-032.pgn","icon-033.pgn","icon-034.pgn","icon-035.pgn","icon-036.pgn","icon-037.pgn","icon-038.pgn","icon-039.pgn",],
["icon-040.pgn","icon-041.pgn","icon-042.pgn","icon-043.pgn","icon-044.pgn","icon-045.pgn","icon-046.pgn","icon-047.pgn",],
["icon-048.pgn","icon-049.pgn","icon-050.pgn","icon-051.pgn","icon-052.pgn","icon-053.pgn","icon-054.pgn","icon-055.pgn",],
];


      this.each(function() {
        var $color = $(this);
        var $item = $color.parent();
        var grid = '';
        var itemWidth = 100 / layout[0].length;
        var itemHeight = 100 / layout.length;
        var boxHeight = itemWidth * layout.length;

        for(var i = 0; i < layout.length; i++) {
          for(var j = 0; j < layout[i].length; j++) {

            var color = layout[i][j];
            var selectable = ' selectable';

            grid += '<i ' +
              'class="color-box" ' +
              'data-value="' + layout[i][j] + '" ' +
              'style="' +
                'width:' + itemWidth + '%; ' +
                'height:' + itemHeight + '%; ' +
                'background:black;">' +
            '<img src="images/' + layout[i][j] + '">'
            '</i>';
          }
        }

        var $injectedColor = $('<div class="item-styled-color">' +
          '<span class="value" style="background:' + $color.val().replace(/^0x/, '#') + '"></span>' +
          '<div ' +
              'style="padding-bottom:' + boxHeight + '%"' +
              'class="color-box-wrap">' +
            '<div class="color-box-container">' +
                grid +
            '</div>' +
          '</div>' +
        '</div>');
        $item.append($injectedColor);

        var $valueDisplay = $injectedColor.find('.value');

        $color.on('click', function(ev) {
          $item.find('.color-box-wrap').toggleClass('show');
        });

        $item.find('.color-box.selectable').on('click', function(ev) {
          ev.preventDefault();

          var value = $(this).data('value');
          $color.val(value);
//           $valueDisplay.css('background-color', value.replace(/^0x/, '#'));
          $item.find('.color-box-wrap').removeClass('show');
        })

      });
    },