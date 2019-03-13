<?php
namespace Hypertable_ThriftGen;

/**
 * Autogenerated by Thrift Compiler (0.12.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
use Thrift\Base\TBase;
use Thrift\Type\TType;
use Thrift\Type\TMessageType;
use Thrift\Exception\TException;
use Thrift\Exception\TProtocolException;
use Thrift\Protocol\TProtocol;
use Thrift\Protocol\TBinaryProtocolAccelerated;
use Thrift\Exception\TApplicationException;

/**
 * State flags for a key
 * 
 * Note for maintainers: the definition must be sync'ed with FLAG_* constants
 * in src/cc/Hypertable/Lib/Key.h
 * 
 * DELETE_ROW: row is pending delete
 * 
 * DELETE_CF: column family is pending delete
 * 
 * DELETE_CELL: key is pending delete
 * 
 * DELETE_CELL_VERSION: delete specific timestamped version of key
 * 
 * INSERT: key is an insert/update (default state)
 */
final class KeyFlag
{
    const DELETE_ROW = 0;

    const DELETE_CF = 1;

    const DELETE_CELL = 2;

    const DELETE_CELL_VERSION = 3;

    const INSERT = 255;

    static public $__names = array(
        0 => 'DELETE_ROW',
        1 => 'DELETE_CF',
        2 => 'DELETE_CELL',
        3 => 'DELETE_CELL_VERSION',
        255 => 'INSERT',
    );
}

