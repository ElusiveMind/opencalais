
$(document).ready(function() {

	$('.calais_keyword').click(function() {
		var tags = $('#' + $(this).attr('for'));
		var keyword = $(this).text();
		
		// TODO: When we move to a revent jQuery, replace is() with hasClass()
		if($(this).is('.calais_keyword_selected')) {
			calaisRemoveKeyword(tags, keyword);
			$(this).removeClass('calais_keyword_selected');
		}
		else {
			calaisAddKeyword(tags, keyword);
			$(this).addClass('calais_keyword_selected');
		}

	});

	$('.calais_keyword').each( function() {
		var tags = $('#' + $(this).attr('for'));
		var keyword = $(this).text();
		
		if (tags.val().indexOf(keyword) != -1) {
			$(this).addClass('calais_keyword_selected');
		}
		
	});
});

/**
 * Insert keyword, adding a comma if necessary
 */
function calaisAddKeyword(tags, keyword) {
	var current = $.trim(tags.val());
	if(current.indexOf(keyword) == -1) {
		if(current == '') {
			tags.val(keyword);				
		}
		else{
			tags.val(current + ',' + keyword);
		}				
	}
}

/**
 * Remove the keyword and cleanup any comma nonsense
 */
function calaisRemoveKeyword(tags, keyword) {
	var current = $.trim(tags.val());
	var index = current.indexOf(keyword); 
	if(index >= 0) {
		// Deal with funky spaces around commas
		current = current.replace(/ +,/, ',');
		current = current.replace(/, +/, ',');
		
		// Remove the keyword
		current = current.replace(keyword, '');
		
		// Deal with a remaining extra comma
		current = current.replace(/^,/, '');
		current = current.replace(/,$/, '');
		current = current.replace(/,,/, ',');
		tags.val(current);
	}
}
