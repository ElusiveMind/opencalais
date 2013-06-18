<h1><?php print $html['title'] ?></h1>;
<?php foreach($html['fields'] as $field_items): ?>
  <div> <?php print implode('</div><div>', $field_items); ?> </div>
<?php endforeach ?>