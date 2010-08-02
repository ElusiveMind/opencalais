Drupal.behaviors.calais_admin = function(context) {

  $("input.nodetype_toggle").change(function(){
    var type = $(this).attr('data');
    var selector = "#" + type + "_toggle";
    if ($(this).val() == "0") {
      $(selector).hide(500);
    }
    else {
      $(selector).show(500);
    }
  });

  $("input.calais-use-global").change(function(){
    var type = $(this).attr('data');
    var selector = ".calais-entity-settings-" + type;
    $(selector).toggle(500);
  });
  
}