<?php

class EntityTypeInfo implements ContainerInjectionInterface {

  use StringTranslationTrait;

  /**
   * Adds OpenCalais links to appropriate entity types.
   *
   * This is an alter hook bridge.
   *
   * @param \Drupal\Core\Entity\EntityTypeInterface[] $entity_types
   *   The master entity type list to alter.
   *
   * @see hook_entity_type_alter()
   */
  public function entityTypeAlter(array &$entity_types) {
    foreach ($entity_types as $entity_type_id => $entity_type) {
      if (($entity_type->getFormClass('default') || $entity_type->getFormClass('edit')) && $entity_type->hasLinkTemplate('edit-form')) {
        $entity_type->setLinkTemplate('devel-load', "/$entity_type_id/{{$entity_type_id}}/opencalais");
      }
    }
  }
}
