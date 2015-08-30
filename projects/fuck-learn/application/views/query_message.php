<?php
defined('BASEPATH') OR exit('No direct script access allowed');
$query = $this->db->query('select * from user');
if ($query->num_rows() > 0)
{
    foreach ($query->result() as $row)
    {
        echo $row->ID;
        echo $row->name;
    }
}