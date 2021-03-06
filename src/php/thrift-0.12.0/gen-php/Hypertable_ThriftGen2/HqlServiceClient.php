<?php
namespace Hypertable_ThriftGen2;

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

class HqlServiceClient extends \Hypertable_ThriftGen\ClientServiceClient implements \Hypertable_ThriftGen2\HqlServiceIf
{
    public function __construct($input, $output = null)
    {
        parent::__construct($input, $output);
    }


    public function hql_exec($ns, $command, $noflush, $unbuffered)
    {
        $this->send_hql_exec($ns, $command, $noflush, $unbuffered);
        return $this->recv_hql_exec();
    }

    public function send_hql_exec($ns, $command, $noflush, $unbuffered)
    {
        $args = new \Hypertable_ThriftGen2\HqlService_hql_exec_args();
        $args->ns = $ns;
        $args->command = $command;
        $args->noflush = $noflush;
        $args->unbuffered = $unbuffered;
        $bin_accel = ($this->output_ instanceof TBinaryProtocolAccelerated) && function_exists('thrift_protocol_write_binary');
        if ($bin_accel) {
            thrift_protocol_write_binary(
                $this->output_,
                'hql_exec',
                TMessageType::CALL,
                $args,
                $this->seqid_,
                $this->output_->isStrictWrite()
            );
        } else {
            $this->output_->writeMessageBegin('hql_exec', TMessageType::CALL, $this->seqid_);
            $args->write($this->output_);
            $this->output_->writeMessageEnd();
            $this->output_->getTransport()->flush();
        }
    }

    public function recv_hql_exec()
    {
        $bin_accel = ($this->input_ instanceof TBinaryProtocolAccelerated) && function_exists('thrift_protocol_read_binary');
        if ($bin_accel) {
            $result = thrift_protocol_read_binary(
                $this->input_,
                '\Hypertable_ThriftGen2\HqlService_hql_exec_result',
                $this->input_->isStrictRead()
            );
        } else {
            $rseqid = 0;
            $fname = null;
            $mtype = 0;

            $this->input_->readMessageBegin($fname, $mtype, $rseqid);
            if ($mtype == TMessageType::EXCEPTION) {
                $x = new TApplicationException();
                $x->read($this->input_);
                $this->input_->readMessageEnd();
                throw $x;
            }
            $result = new \Hypertable_ThriftGen2\HqlService_hql_exec_result();
            $result->read($this->input_);
            $this->input_->readMessageEnd();
        }
        if ($result->success !== null) {
            return $result->success;
        }
        if ($result->e !== null) {
            throw $result->e;
        }
        throw new \Exception("hql_exec failed: unknown result");
    }

    public function hql_query($ns, $command)
    {
        $this->send_hql_query($ns, $command);
        return $this->recv_hql_query();
    }

    public function send_hql_query($ns, $command)
    {
        $args = new \Hypertable_ThriftGen2\HqlService_hql_query_args();
        $args->ns = $ns;
        $args->command = $command;
        $bin_accel = ($this->output_ instanceof TBinaryProtocolAccelerated) && function_exists('thrift_protocol_write_binary');
        if ($bin_accel) {
            thrift_protocol_write_binary(
                $this->output_,
                'hql_query',
                TMessageType::CALL,
                $args,
                $this->seqid_,
                $this->output_->isStrictWrite()
            );
        } else {
            $this->output_->writeMessageBegin('hql_query', TMessageType::CALL, $this->seqid_);
            $args->write($this->output_);
            $this->output_->writeMessageEnd();
            $this->output_->getTransport()->flush();
        }
    }

    public function recv_hql_query()
    {
        $bin_accel = ($this->input_ instanceof TBinaryProtocolAccelerated) && function_exists('thrift_protocol_read_binary');
        if ($bin_accel) {
            $result = thrift_protocol_read_binary(
                $this->input_,
                '\Hypertable_ThriftGen2\HqlService_hql_query_result',
                $this->input_->isStrictRead()
            );
        } else {
            $rseqid = 0;
            $fname = null;
            $mtype = 0;

            $this->input_->readMessageBegin($fname, $mtype, $rseqid);
            if ($mtype == TMessageType::EXCEPTION) {
                $x = new TApplicationException();
                $x->read($this->input_);
                $this->input_->readMessageEnd();
                throw $x;
            }
            $result = new \Hypertable_ThriftGen2\HqlService_hql_query_result();
            $result->read($this->input_);
            $this->input_->readMessageEnd();
        }
        if ($result->success !== null) {
            return $result->success;
        }
        if ($result->e !== null) {
            throw $result->e;
        }
        throw new \Exception("hql_query failed: unknown result");
    }

    public function hql_exec_as_arrays($ns, $command, $noflush, $unbuffered)
    {
        $this->send_hql_exec_as_arrays($ns, $command, $noflush, $unbuffered);
        return $this->recv_hql_exec_as_arrays();
    }

    public function send_hql_exec_as_arrays($ns, $command, $noflush, $unbuffered)
    {
        $args = new \Hypertable_ThriftGen2\HqlService_hql_exec_as_arrays_args();
        $args->ns = $ns;
        $args->command = $command;
        $args->noflush = $noflush;
        $args->unbuffered = $unbuffered;
        $bin_accel = ($this->output_ instanceof TBinaryProtocolAccelerated) && function_exists('thrift_protocol_write_binary');
        if ($bin_accel) {
            thrift_protocol_write_binary(
                $this->output_,
                'hql_exec_as_arrays',
                TMessageType::CALL,
                $args,
                $this->seqid_,
                $this->output_->isStrictWrite()
            );
        } else {
            $this->output_->writeMessageBegin('hql_exec_as_arrays', TMessageType::CALL, $this->seqid_);
            $args->write($this->output_);
            $this->output_->writeMessageEnd();
            $this->output_->getTransport()->flush();
        }
    }

    public function recv_hql_exec_as_arrays()
    {
        $bin_accel = ($this->input_ instanceof TBinaryProtocolAccelerated) && function_exists('thrift_protocol_read_binary');
        if ($bin_accel) {
            $result = thrift_protocol_read_binary(
                $this->input_,
                '\Hypertable_ThriftGen2\HqlService_hql_exec_as_arrays_result',
                $this->input_->isStrictRead()
            );
        } else {
            $rseqid = 0;
            $fname = null;
            $mtype = 0;

            $this->input_->readMessageBegin($fname, $mtype, $rseqid);
            if ($mtype == TMessageType::EXCEPTION) {
                $x = new TApplicationException();
                $x->read($this->input_);
                $this->input_->readMessageEnd();
                throw $x;
            }
            $result = new \Hypertable_ThriftGen2\HqlService_hql_exec_as_arrays_result();
            $result->read($this->input_);
            $this->input_->readMessageEnd();
        }
        if ($result->success !== null) {
            return $result->success;
        }
        if ($result->e !== null) {
            throw $result->e;
        }
        throw new \Exception("hql_exec_as_arrays failed: unknown result");
    }

    public function hql_exec2($ns, $command, $noflush, $unbuffered)
    {
        $this->send_hql_exec2($ns, $command, $noflush, $unbuffered);
        return $this->recv_hql_exec2();
    }

    public function send_hql_exec2($ns, $command, $noflush, $unbuffered)
    {
        $args = new \Hypertable_ThriftGen2\HqlService_hql_exec2_args();
        $args->ns = $ns;
        $args->command = $command;
        $args->noflush = $noflush;
        $args->unbuffered = $unbuffered;
        $bin_accel = ($this->output_ instanceof TBinaryProtocolAccelerated) && function_exists('thrift_protocol_write_binary');
        if ($bin_accel) {
            thrift_protocol_write_binary(
                $this->output_,
                'hql_exec2',
                TMessageType::CALL,
                $args,
                $this->seqid_,
                $this->output_->isStrictWrite()
            );
        } else {
            $this->output_->writeMessageBegin('hql_exec2', TMessageType::CALL, $this->seqid_);
            $args->write($this->output_);
            $this->output_->writeMessageEnd();
            $this->output_->getTransport()->flush();
        }
    }

    public function recv_hql_exec2()
    {
        $bin_accel = ($this->input_ instanceof TBinaryProtocolAccelerated) && function_exists('thrift_protocol_read_binary');
        if ($bin_accel) {
            $result = thrift_protocol_read_binary(
                $this->input_,
                '\Hypertable_ThriftGen2\HqlService_hql_exec2_result',
                $this->input_->isStrictRead()
            );
        } else {
            $rseqid = 0;
            $fname = null;
            $mtype = 0;

            $this->input_->readMessageBegin($fname, $mtype, $rseqid);
            if ($mtype == TMessageType::EXCEPTION) {
                $x = new TApplicationException();
                $x->read($this->input_);
                $this->input_->readMessageEnd();
                throw $x;
            }
            $result = new \Hypertable_ThriftGen2\HqlService_hql_exec2_result();
            $result->read($this->input_);
            $this->input_->readMessageEnd();
        }
        if ($result->success !== null) {
            return $result->success;
        }
        if ($result->e !== null) {
            throw $result->e;
        }
        throw new \Exception("hql_exec2 failed: unknown result");
    }

    public function hql_query_as_arrays($ns, $command)
    {
        $this->send_hql_query_as_arrays($ns, $command);
        return $this->recv_hql_query_as_arrays();
    }

    public function send_hql_query_as_arrays($ns, $command)
    {
        $args = new \Hypertable_ThriftGen2\HqlService_hql_query_as_arrays_args();
        $args->ns = $ns;
        $args->command = $command;
        $bin_accel = ($this->output_ instanceof TBinaryProtocolAccelerated) && function_exists('thrift_protocol_write_binary');
        if ($bin_accel) {
            thrift_protocol_write_binary(
                $this->output_,
                'hql_query_as_arrays',
                TMessageType::CALL,
                $args,
                $this->seqid_,
                $this->output_->isStrictWrite()
            );
        } else {
            $this->output_->writeMessageBegin('hql_query_as_arrays', TMessageType::CALL, $this->seqid_);
            $args->write($this->output_);
            $this->output_->writeMessageEnd();
            $this->output_->getTransport()->flush();
        }
    }

    public function recv_hql_query_as_arrays()
    {
        $bin_accel = ($this->input_ instanceof TBinaryProtocolAccelerated) && function_exists('thrift_protocol_read_binary');
        if ($bin_accel) {
            $result = thrift_protocol_read_binary(
                $this->input_,
                '\Hypertable_ThriftGen2\HqlService_hql_query_as_arrays_result',
                $this->input_->isStrictRead()
            );
        } else {
            $rseqid = 0;
            $fname = null;
            $mtype = 0;

            $this->input_->readMessageBegin($fname, $mtype, $rseqid);
            if ($mtype == TMessageType::EXCEPTION) {
                $x = new TApplicationException();
                $x->read($this->input_);
                $this->input_->readMessageEnd();
                throw $x;
            }
            $result = new \Hypertable_ThriftGen2\HqlService_hql_query_as_arrays_result();
            $result->read($this->input_);
            $this->input_->readMessageEnd();
        }
        if ($result->success !== null) {
            return $result->success;
        }
        if ($result->e !== null) {
            throw $result->e;
        }
        throw new \Exception("hql_query_as_arrays failed: unknown result");
    }

    public function hql_query2($ns, $command)
    {
        $this->send_hql_query2($ns, $command);
        return $this->recv_hql_query2();
    }

    public function send_hql_query2($ns, $command)
    {
        $args = new \Hypertable_ThriftGen2\HqlService_hql_query2_args();
        $args->ns = $ns;
        $args->command = $command;
        $bin_accel = ($this->output_ instanceof TBinaryProtocolAccelerated) && function_exists('thrift_protocol_write_binary');
        if ($bin_accel) {
            thrift_protocol_write_binary(
                $this->output_,
                'hql_query2',
                TMessageType::CALL,
                $args,
                $this->seqid_,
                $this->output_->isStrictWrite()
            );
        } else {
            $this->output_->writeMessageBegin('hql_query2', TMessageType::CALL, $this->seqid_);
            $args->write($this->output_);
            $this->output_->writeMessageEnd();
            $this->output_->getTransport()->flush();
        }
    }

    public function recv_hql_query2()
    {
        $bin_accel = ($this->input_ instanceof TBinaryProtocolAccelerated) && function_exists('thrift_protocol_read_binary');
        if ($bin_accel) {
            $result = thrift_protocol_read_binary(
                $this->input_,
                '\Hypertable_ThriftGen2\HqlService_hql_query2_result',
                $this->input_->isStrictRead()
            );
        } else {
            $rseqid = 0;
            $fname = null;
            $mtype = 0;

            $this->input_->readMessageBegin($fname, $mtype, $rseqid);
            if ($mtype == TMessageType::EXCEPTION) {
                $x = new TApplicationException();
                $x->read($this->input_);
                $this->input_->readMessageEnd();
                throw $x;
            }
            $result = new \Hypertable_ThriftGen2\HqlService_hql_query2_result();
            $result->read($this->input_);
            $this->input_->readMessageEnd();
        }
        if ($result->success !== null) {
            return $result->success;
        }
        if ($result->e !== null) {
            throw $result->e;
        }
        throw new \Exception("hql_query2 failed: unknown result");
    }
}
